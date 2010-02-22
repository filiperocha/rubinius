require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/classes', __FILE__)

describe "Thread::list" do
  it "includes the current and main thread" do
    Thread.list.should include(Thread.current)
    Thread.list.should include(Thread.main)
  end

  it "does not include deceased threads" do
    t = Thread.new { 1; }
    t.join
    Thread.list.should_not include(t)
  end

  it "includes waiting threads" do
    c = Channel.new
    t = Thread.new { c.receive }
    begin
      Thread.pass while t.status and t.status != 'sleep'
      Thread.list.should include(t)
    ensure
      c << nil
      t.join
    end
  end
end
