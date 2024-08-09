# Contribution Guide

Welcome to the iLogtail community! Thank you for contributing code, documentation, and cases to iLogtail!

Since its open-source release, iLogtail has gained significant attention from the community. Every issue, pull request (PR), is a brick in the development of iLogtail. We sincerely hope that more community members will join the project and help make iLogtail even better together.

## Code of Conduct

When participating in the iLogtail community, please read and agree to the [Alibaba Open Source Code of Conduct](https://github.com/alibaba/community/blob/master/CODE_OF_CONDUCT_zh.md) to create an open, transparent, and friendly open-source environment.

## Contribution Process

We are always open to contributions, whether it's fixing typos, bug fixes, adding new features, or engaging in community interactions in any form, such as code, documentation, or case studies. We also encourage contributions to other open-source projects.

### Getting Started

If you're new to contributing, start with the [good first issue](https://github.com/alibaba/ilogtail/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22) list to quickly become involved in the community.

1. **Claim an issue:** Simply reply to the relevant issue expressing your interest, then follow the GitHub workflow instructions below to resolve the issue and submit a PR according to the guidelines. Once reviewed, your changes will be merged into the main branch.

## GitHub Workflow

We use the main branch as our development branch, which represents an unstable version. Stable releases are archived in the [Releases](https://github.com/alibaba/ilogtail/releases) section, with each release tagged with its corresponding version.

### Creating/Claiming Issues

We use GitHub [Issues](https://github.com/alibaba/ilogtail/issues) and [Pull Requests](https://github.com/alibaba/ilogtail/pulls) to manage and track requirements or issues. For new features, bug fixes, documentation updates, or suggestions, create an issue; you can also claim existing issues we've released.

1. Fill out the issue template to ensure we understand your request clearly. Also, tag the issue appropriately for easy management.

### Design Discussion

For significant changes (like module restructuring or adding new components), **it's essential to initiate a detailed discussion and write a comprehensive design document**. This should outline the problem being solved, the purpose, and the design. Reach a consensus before making the changes. You can propose designs in the Issue or [Discussions ideas](https://github.com/alibaba/ilogtail/discussions/categories/ideas) section. For support, join our钉钉群。

For smaller changes, briefly describe your design in the Issue.

### Development & Testing

Once the design is finalized, you can proceed with the development process. Here's a common workflow for open-source contributors:

1. Fork the [iLogtail](https://github.com/alibaba/ilogtail) repository to your personal GitHub account.
2. Develop and test on a personal fork branch, following these steps:
   - Keep your personal main branch in sync with the iLogtail main repository.
   - Clone your forked repository locally.
   - Create a new development branch, develop, and test. Ensure all changes have unit tests or E2E tests.
   - Commit changes with a concise and well-formatted commit message, using the same email as your GitHub account.
   - Push the changes to your remote branch.
3. Create a [pull request (PR)](https://github.com/alibaba/ilogtail/pulls) from your personal branch to the main branch. For significant changes, ensure there's a corresponding issue, and link them.

Before submitting a PR, follow these guidelines:

- [Code/Document Style](../developer-guide/codestyle.md)
- [Coding Standards](../developer-guide/code-check/check-codestyle.md)
- [Dependency Licensing](../developer-guide/code-check/check-dependency-license.md)
- [File Licensing](../developer-guide/code-check/check-license.md)
- For major features or bug fixes, include a [Changelog](https://github.com/alibaba/ilogtail/blob/main/CHANGELOG.md)

Note that a PR should not be too large. If necessary, break it into smaller, separate PRs for better management.

If you're a first-time contributor, sign the CLA (a link will appear in the PR prompt). Ensure all continuous integration (CI) tests pass before submitting. A reviewer will be assigned, and once approved, your changes will be merged.

When merging a PR, squash multiple commits into one to maintain a concise and well-formatted commit history.

## Labels

We use labels to manage Issues, PRs, and Discussions. Add the appropriate label based on the situation:

- New features: `feature request`
- Enhancements to existing features: `enhancement`
- Good for beginners: `good first issue`
- Bugs: `bug`
- Testing framework or test cases: `test`
- Config examples or K8s deployments: [config](#config)
- Documentation: `documentation`
- Case studies: [case](#case)
- Questions and answers: `question`. We recommend discussing in [Discussions](https://github.com/alibaba/ilogtail/discussions) first.

Additional labels:

- Community contributions: `community`
- Core functionality development or changes: `core`

## Code Review

All code must go through a committer's review. Here are some recommended principles:

- Readability: Follow our [code style](../developer-guide/codestyle.md) and [document style](../developer-guide/plugin-doc-templete.md).
- Elegance: Keep code concise, reusable, and well-designed.
- Testing: Important code should have comprehensive test cases (unit tests, E2E tests), with a focus on test coverage.

## Config Sharing <a href="#config" id="config"></a>

In addition to sharing code, you can contribute to our configuration templates:

- The `example_config/data_pipelines` directory contains scenario-based data pipeline templates, highlighting processor and aggregator plugin functionality. If you have insights on processing common logs (like Apache, Spring Boot), refer to existing samples and the [README](https://github.com/alibaba/ilogtail/tree/main/example_config#readme) to contribute.

- The `k8s_templates` directory contains complete K8s deployment examples, dividing them by input and flusher plugins. If you have expertise in deploying common log processing scenarios (like sending data to Elasticsearch, Clickhouse), refer to existing samples and the [README](https://github.com/alibaba/ilogtail/tree/main/k8s_templates#readme) to contribute.

## Case Sharing <a href="#case" id="case"></a>

We welcome sharing any iLogtail use cases. We've set up a column on Zhihu called [iLogtail Community](https://www.zhihu.com/column/c_1533139823409270785), where you can submit articles to share your experiences with iLogtail.

1. Write articles on Zhihu, such as [A Comprehensive Guide to SAE Logging Collection Architecture](https://zhuanlan.zhihu.com/p/557591446).
2. Recommend your articles to the "iLogtail Community" column.
3. Modify the [use-cases.md](https://github.com/alibaba/ilogtail/blob/main/docs/cn/awesome-ilogtail/use-cases.md) file on GitHub and submit a PR, tagging it with `awesome ilogtail`.

## Engage in Community Discussion

Feel free to discuss any issues you encounter while using iLogtail in the [Discussions](https://github.com/alibaba/ilogtail/discussions). You can also help others by answering questions there.

### Discussion Categories

- Announcements: iLogtail official announcements.
- Help: Seek assistance in using iLogtail.
- Ideas: Share ideas about iLogtail; discussions are always welcome.
- Show and tell: Showcase any work related to iLogtail, such as small tools.

## Contact Us

![image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/chatgroup.png)
