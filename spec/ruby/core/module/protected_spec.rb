require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/classes', __FILE__)

describe "Module#protected" do
  
  before :each do
    class << ModuleSpecs::Parent
      def protected_method_1; 5; end
    end
  end
  
  it "makes an existing class method protected" do
    ModuleSpecs::Parent.protected_method_1.should == 5
    
    class << ModuleSpecs::Parent
      protected :protected_method_1
    end
    
    lambda { ModuleSpecs::Parent.protected_method_1 }.should raise_error(NoMethodError)
  end
  
end

