require "test_helper"

# noinspection RubyInstanceMethodNamingConvention
class FastMatrixGemTest < Minitest::Test
  def test_that_it_has_a_version_number
    refute_nil ::FastMatrix::VERSION
  end
end
