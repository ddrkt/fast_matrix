[![Build Status](https://travis-ci.org/mmcs-ruby/fast_matrix.svg?branch=master)](https://travis-ci.org/mmcs-ruby/fast_matrix)
[![Maintainability](https://api.codeclimate.com/v1/badges/fd171bae2ca444aaca29/maintainability)](https://codeclimate.com/github/mmcs-ruby/fast_matrix/maintainability)
[![Test Coverage](https://api.codeclimate.com/v1/badges/fd171bae2ca444aaca29/test_coverage)](https://codeclimate.com/github/mmcs-ruby/fast_matrix/test_coverage)

# FastMatrix

Ruby wrapper around C matrices implementation written as exercise by MMCS students.

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'fast_matrix'
```

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install fast_matrix

## Usage and documentation

See our [GitHub Wiki](https://github.com/mmcs-ruby/fast_matrix/wiki).

## Development

Fast matrix is native extension in C, so, for development you need Linux (maybe, MacOS) or Docker.

After checking out the repo, run `gem install bundler -v 2.0.2`, `bundle install` and `bundler exec rake` or use `Dockerfile`.

Commands:
  + `bundler exec rake compile` - compile C part of gem;
  + `bundler exec rake test` - run tests;
  + `bundler exec rake test TESTOPTS='-v'` - run tests with more information (description skipped tests);
  + `bundler exec rake` - compile and run tests.


To install this gem onto your local machine, run `bundle exec rake install`. To release a new version, update the version number in `version.rb`, and then run `bundle exec rake release`, which will create a git tag for the version, push git commits and tags, and push the `.gem` file to [rubygems.org](https://rubygems.org).

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/mmcs-ruby/fast_matrix. This project is intended to be a safe, welcoming space for collaboration, and contributors are expected to adhere to the [Contributor Covenant](http://contributor-covenant.org) code of conduct.

## License

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT).

## Code of Conduct

Everyone interacting in the FastMatrix project’s codebases, issue trackers, chat rooms and mailing lists is expected to follow the [code of conduct](https://github.com/mmcs-ruby/fast_matrix/blob/master/CODE_OF_CONDUCT.md).
