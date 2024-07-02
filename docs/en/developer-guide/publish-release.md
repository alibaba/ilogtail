# Release a Version

1. **Check** the `CHANGELOG.md` in the root directory for the "Unreleased" section.

2. **Run** the script `sh scripts/gen_release_markdown.sh {version} {milestone-number}`, where `{version}` is the version number (e.g., v1.0.28) and `{milestone-number}` can be found [here](https://github.com/alibaba/ilogtail/milestones). For example, if the milestone for v1.0.28 is [2](https://github.com/alibaba/ilogtail/milestone/2), execute `sh scripts/gen_release_markdown.sh 1.0.28 2`.

3. **Verify** that the release notes generated in `changes/{version}.md` contain all the pending release content from the first point.

4. **Test** the download link in the generated release notes to ensure it is accessible. If not, proceed with uploading the release version to OSS.

5. **Remove** the "Unreleased" section from the `CHANGELOG.md`.

6. **Commit** and **push** the changes to GitHub.

7. **Release** through the interface, copying the content from `changes/{version}.md`.

---

Note: The Markdown formatting has been preserved in the translation.
