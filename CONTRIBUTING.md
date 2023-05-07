# Introduction

Thank you for considering contributing to Klogg.
It's people like you that make Klogg such a great tool.

Following these guidelines helps to communicate that you respect 
the time of the developers managing and developing this open source project.
In return, they should reciprocate that respect in addressing your issue,
assessing changes, and helping you finalize your pull requests.

There are many ways to contribute, from writing tutorials or blog posts,
improving the documentation, submitting bug reports and feature requests
to writing code which can be incorporated into Klogg itself.

# Ground Rules

 * Keep pull requests and issues as small as possible, preferably one new feature or bug description per request.
 * Ensure cross-platform compatibility for every code change: Windows, Mac, Ubuntu Linux.
 * Create issues for any major changes and enhancements that you wish to make. Discuss things transparently and get community feedback.
 * Be welcoming to newcomers and encourage diverse new contributors from all backgrounds.
  See the [Code of Conduct](CODE_OF_CONDUCT.md).

# How to suggest a feature or enhancement

Klogg is intended to be a log *viewing* tool with additional features that help
navigate through text files, extract information and reconstruct chain of events.

If you find yourself wishing for a feature that doesn't exist in Klogg,
you are probably not alone. There are bound to be others out there with similar needs.
Many of the features that Klogg has today have been added because our users saw the need.
Open an issue on GitHub which describes the feature you would like to see,
why you need it, and how it should work.

# How to report a bug

If you find a security vulnerability, do **NOT** open an issue. 

In order to determine whether you are dealing with a security issue, ask yourself these two questions:
 * Can I access something that's not mine, or something I shouldn't have access to?
 * Can I disable something for other people?

 If the answer to either of those two questions are "yes", then you're probably dealing with a security issue.
 Note that even if you answer "no" to both questions, you may still be dealing with a security issue,
 so if you're unsure, just email us at klogg@filimonov.dev.

When filing an issue, make sure to answer these five questions:

1. What version of Klogg are you using (version is listed in window title and about dialog)?
1. What operating system are you using?
1. What did you do?
1. What did you expect to see?
1. What did you see instead?

General questions do not need to follow this checklist. 
Feel free to ask anything about using, developing or distributing Klogg. Such questions often
help to improve project documentation.

# Documentation

Klogg has become a quite complex tool with many features. Any time spent fixing
typos or clarifying sections in the documentation is greatly appreciated.
Features that need better documentation can be found in this 
[list](https://github.com/variar/klogg/issues?q=is%3Aissue+label%3A%22status%3A+need+documentation%22+). 
Both open and closed issues marked with label `status: need documentation`
require some work with documentation.

# How to contribute code

Unsure where to begin contributing to Klogg? 
You can start by looking through these issues:
- [Good first issues](https://github.com/variar/klogg/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22+sort%3Acomments-desc) -
 issues which should only require a few lines of code.
- [Help wanted issues](https://github.com/variar/klogg/issues?q=is%3Aissue+is%3Aopen+sort%3Acomments-desc+label%3A%22help+wanted%22) -
 issues which should be a bit more involved, required some discussion.

Both issue lists are sorted by total number of comments. While not perfect, number of comments is a reasonable proxy for impact a given change will have.

Working on your first Pull Request? You can learn how from this free series, [How to Contribute to an Open Source Project on GitHub](https://egghead.io/series/how-to-contribute-to-an-open-source-project-on-github).

At this point, you're ready to make your changes! Feel free to ask for help; everyone is a beginner at first :smile_cat:

For something that is bigger than a ten line fix:

1. Create an issue to discuss you idea. It's generally best if you get confirmation
 of your bug fix or approval for your feature
 request this way before starting to code.
1. Create your own fork of the code
1. Do the changes in your fork
1. If you like the change and think the project could use it:
    * Be sure you have followed the code style for the project (.clang-format file is provided)
    * Note the [Code of Conduct](CODE_OF_CONDUCT.md).
    * Create a pull request

## Commit message format
If possible commit message should be like `prefix: message`, where prefix is one of
```
  feat = 'Features',
  fix = 'Bug Fixes',
  docs = 'Documentation',
  style = 'Styles',
  refactor = 'Code Refactoring',
  perf = 'Performance Improvements',
  test = 'Tests',
  build = 'Builds',
  ci = 'Continuous Integration',
  chore = 'Chores',
  revert = 'Reverts',
  tr = 'Translations'
```
    