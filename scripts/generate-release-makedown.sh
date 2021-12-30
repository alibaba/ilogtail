ROOTDIR=$(cd $(dirname $0) && cd ..&& pwd)


function createReleaseFile () {
    version=$1
    mileston=$2
    if [ ! -d "${ROOTDIR}/changes" ]; then
        mkdir ${ROOTDIR}/changes
    fi
    doc=${ROOTDIR}/changes/v${version}.md
    if [ -f "${doc}" ]; then
        rm -rf ${doc}
    fi
    touch $doc
    echo "# ${version}" >> $doc
    echo "## Changes" >> $doc
    echo  "All issues and pull requests are [here](https://github.com/alibaba/ilogtail/milestone/${mileston})." >> $doc
    appendUnreleaseChanges $doc

     echo "## Download" >> $doc
    appendDownloadLinks $doc $version
}


function appendUnreleaseChanges () {
    doc=$1
    changeDoc=$ROOTDIR/CHANGELOG.md
    tempFile=$(mktemp temp.XXXXXX)
    cat $changeDoc|grep "Unreleased" -A 500 |grep -v "Unreleased"|grep -v '^$' >> $tempFile
    echo "### Features" >> $doc
    cat $tempFile|grep -E '\[added\]|\[updated\]|\[deprecated\]|\[removed\]' >> $doc
    echo "### Fixed" >> $doc
    cat $tempFile|grep -E '\[fixed\]|\[security\]' >> $doc
    echo "### Doc" >> $doc
    cat $tempFile|grep -E '\[doc\]' >> $doc
    rm -rf $tempFile
}

function appendDownloadLinks () {
    doc=$1
    version=$2
    echo "| Arch| Platform| Region| Link|" >> $doc
    echo "|  ----  | ----  | ----  | ----  |" >> $doc
    echo "|arm64|Linux|China|[link](http://logtail-release-cn-hangzhou.oss-cn-hangzhou.aliyuncs.com/linux64/${version}/aarch64/logtail-linux64.tar.gz)|" >> $doc
    echo "|amd64|Linux|China|[link](http://logtail-release-cn-hangzhou.oss-cn-hangzhou.aliyuncs.com/linux64/${version}/x86_64/logtail-linux64.tar.gz)" >> $doc
    echo "|arm64|Linux|US|[link](http://logtail-release-us-west-1.oss-us-west-1.aliyuncs.com/linux64/${version}/aarch64/logtail-linux64.tar.gz)" >> $doc
    echo "|amd64|Linux|US|[link](http://logtail-release-us-west-1.oss-us-west-1.aliyuncs.com/linux64/${version}/x86_64/logtail-linux64.tar.gz)" >> $doc
}


function removeHistoryUnrelease () {
    changeDoc=$ROOTDIR/CHANGELOG.md
    tempFile=$(mktemp temp.XXXXXX)
    cat $changeDoc|grep "Unreleased" -B 500 > $tempFile
    cat $tempFile > $ROOTDIR/CHANGELOG.md
    rm -rf $tempFile
}

version=$1
mileston=$2
createReleaseFile $version $mileston
removeHistoryUnrelease

