#include "prelude.hpp"
#include "builtin/proc.hpp"
#include "vm/object_utils.hpp"
#include "objectmemory.hpp"
#include "builtin/class.hpp"
#include "builtin/block_environment.hpp"
#include "builtin/compiledmethod.hpp"
#include "builtin/system.hpp"
#include "builtin/location.hpp"
#include "builtin/nativemethod.hpp"

#include "arguments.hpp"

#include "vmmethod.hpp"
#include "arguments.hpp"
#include "dispatch.hpp"
#include "call_frame.hpp"

namespace rubinius {

  void Proc::init(STATE) {
    GO(proc).set(state->new_class("Proc", G(object)));
    G(proc)->set_object_type(state, ProcType);
  }

  Proc* Proc::create(STATE, Object* self) {
    return state->new_object<Proc>(as<Class>(self));
  }

  Proc* Proc::from_env(STATE, Object* self, Object* env) {
    if(Proc* p = try_as<Proc>(env)) {
      return p;
    }

    if(BlockEnvironment* be = try_as<BlockEnvironment>(env)) {
      Proc* proc = Proc::create(state, self);
      proc->block(state, be);
      return proc;
    }

    return reinterpret_cast<Proc*>(Primitives::failure());
  }

  Object* Proc::call(STATE, CallFrame* call_frame, Arguments& args) {
    bool lambda_style = RTEST(lambda_);
    int flags = 0;

    // Check the arity in lambda mode
    if(lambda_style) {
      flags = CallFrame::cIsLambda;
      int required = block_->code()->required_args()->to_native();

      bool arity_ok = false;
      if(Fixnum* fix = try_as<Fixnum>(block_->code()->splat())) {
        if(fix->to_native() == -1) {
          // splat = -1 is used to distinguish { |a, | } from { |a| }
          if(args.total() == (size_t)required) arity_ok = true;
        } else if(fix->to_native() == -2) {
          arity_ok = true;
        } else if(args.total() >= (size_t)required) {
          arity_ok = true;
        }

      /* For blocks taking one argument { |a|  }, in 1.8, there is a warning
       * issued but no exception raised when less than or more than one
       * argument is passed. If more than one is passed, 'a' receives an Array
       * of all the arguments.
       */
      } else if(required == 1) {
        arity_ok = true;
      } else {
        arity_ok = ((size_t)required == args.total());
      }

      if(!arity_ok) {
        Exception* exc =
          Exception::make_argument_error(state, required, args.total(), state->symbol("__block__"));
        exc->locations(state, Location::from_call_stack(state, call_frame));
        state->thread_state()->raise_exception(exc);
        return NULL;
      }
    }

    Object* ret;
    if(bound_method_->nil_p()) {
      ret = block_->call(state, call_frame, args, flags);
    } else if(NativeMethod* nm = try_as<NativeMethod>(bound_method_)) {
      Dispatch dis(state->symbol("call"));
      dis.method = nm;
      dis.module = G(rubinius);
      ret = nm->execute(state, call_frame, dis, args);
    } else {
      Dispatch dis(state->symbol("__yield__"));
      ret = dis.send(state, call_frame, args);
    }

    return ret;
  }

  Object* Proc::yield(STATE, CallFrame* call_frame, Arguments& args) {
    if(bound_method_->nil_p()) {
      // NOTE! To match MRI semantics, this explicitely ignores lambda_.
      return block_->call(state, call_frame, args, 0);
    } else if(NativeMethod* nm = try_as<NativeMethod>(bound_method_)) {
      Dispatch dis(state->symbol("call"));
      dis.method = nm;
      dis.module = G(rubinius);
      return nm->execute(state, call_frame, dis, args);
    } else {
      Dispatch dis;
      return call_prim(state, NULL, call_frame, dis, args);
    }
  }

  Object* Proc::call_prim(STATE, Executable* exec, CallFrame* call_frame, Dispatch& msg,
                          Arguments& args) {
    return call(state, call_frame, args);
  }

  Object* Proc::call_on_object(STATE, Executable* exec, CallFrame* call_frame,
                               Dispatch& msg, Arguments& args) {
    bool lambda_style = !lambda_->nil_p();
    int flags = 0;

    // Check the arity in lambda mode
    if(lambda_style) {
      flags = CallFrame::cIsLambda;
      int required = block_->code()->required_args()->to_native();

      if(args.total() < 1 || (required >= 0 && (size_t)required != args.total() - 1)) {
        Exception* exc =
          Exception::make_argument_error(state, required, args.total(), state->symbol("__block__"));
        exc->locations(state, Location::from_call_stack(state, call_frame));
        state->thread_state()->raise_exception(exc);
        return NULL;
      }
    }

    // Since we allow Proc's to be created without setting block_, we need to check
    // it.
    if(block_->nil_p()) {
      Exception* exc =
        Exception::make_type_error(state, BlockEnvironment::type, block_, "Invalid proc style");
      exc->locations(state, Location::from_call_stack(state, call_frame));
      state->thread_state()->raise_exception(exc);
      return NULL;
    }

    return block_->call_on_object(state, call_frame, args, flags);
  }
}
