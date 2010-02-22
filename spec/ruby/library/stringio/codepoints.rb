# -*- encoding: utf-8 -*-
require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/classes', __FILE__)
require File.expand_path('../shared/codepoints', __FILE__)

# See redmine #1667
describe "StringIO#codepoints" do
  ruby_version_is "1.9" do
    it_behaves_like(:stringio_codepoints, :codepoints)
  end
end
