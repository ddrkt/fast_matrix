# frozen_string_literal: true
require 'test_helper'

module FastMatrixTest
  # noinspection RubyInstanceMethodNamingConvention
  class AlgebraTest < Minitest::Test
    def test_multiply_mm
      m1 = FastMatrix::Matrix[[1, 2],
                              [3, 4],
                              [7, 0],
                              [-3, 1]]

      m2 = FastMatrix::Matrix[[1, 0, 3],
                              [4, 5, -2]]

      expected = FastMatrix::Matrix[[9, 10, -1],
                                    [19, 20, 1],
                                    [7, 0, 21],
                                    [1, 5, -11]]
      assert_equal expected, m1 * m2
    end

    def test_multiply_mn
      m = FastMatrix::Matrix[[1, 2], [3, 4], [7, 0], [-3, 1]]
      expected = FastMatrix::Matrix[[5, 10], [15, 20], [35, 0], [-15, 5]]

      assert_equal expected, m * 5
    end

    def test_multiply_mn_commutative
      skip 'Issue#7'
      m = FastMatrix::Matrix[[1, 2], [3, 4], [7, 0], [-3, 1]]
      assert_equal m * 5, 5 * m
    end

    def test_multiply_mv
      m = FastMatrix::Matrix[[1, 2], [3, 4], [7, 0], [-3, 1]]
      v = FastMatrix::Vector[5, 10]
      expected = FastMatrix::Vector[25, 55, 35, -5]

      assert_equal expected, m * v
    end

    def test_transpose
      m = FastMatrix::Matrix[[1, 2], [3, 4], [7, 0]]
      expected = FastMatrix::Matrix[[1, 3, 7], [2, 4, 0]]

      assert_equal expected, m.transpose
    end

    def test_sum
      m1 = FastMatrix::Matrix[[1, -2], [3, 4], [7, 0]]
      m2 = FastMatrix::Matrix[[4, 0], [-3, 4], [2, 2]]
      expected = FastMatrix::Matrix[[5, -2], [0, 8], [9, 2]]

      assert_equal expected, m1 + m2
    end

    def test_sum_with_assigment
      m = FastMatrix::Matrix[[1, -2], [3, 4], [7, 0]]
      m += FastMatrix::Matrix[[4, 0], [-3, 4], [2, 2]]
      expected = FastMatrix::Matrix[[5, -2], [0, 8], [9, 2]]

      assert_equal expected, m
    end
  end
end
