module FastMatrix
  # From C:
  #   TypeError
  #   IndexError

  class Error < StandardError; end
  class NotSupportedError < NotImplementedError; end

end
