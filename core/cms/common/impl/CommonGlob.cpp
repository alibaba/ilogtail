#ifndef WIN32
#include "common/CommonGlob.h"
#include <glob.h> 
#include <string.h> 
#include <stdexcept>
#include <fnmatch.h>
#include "common/Logger.h"
#include "common/StringUtils.h"
namespace common{
using namespace std;
void globUtil::myglob1(const string &pattern, vector<std::string> &fileNames) {
    if(StringUtils::StartWith(pattern,".")){
        return;
    }
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    int rv = glob(pattern.c_str(), GLOB_NOSORT|GLOB_NOESCAPE|GLOB_MARK, NULL, &glob_result);
    if(rv != 0 && rv != GLOB_NOMATCH) {
        globfree(&glob_result);
        LogInfo("glob() ({}) failed with return value: {}",pattern, rv);
        return;
    }
    for(size_t i = 0; i < glob_result.gl_pathc; ++i) {
        fileNames.push_back(string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
}
void globUtil::adjustPattern(const string &pattern, vector<string> &patterns){
    string newPattern=pattern;
    int pos=0;
    while((pos=newPattern.find("**",pos))!=string::npos){
        int tmpPos = pos+2;
        for(;tmpPos<newPattern.size()&&newPattern[tmpPos]=='*';tmpPos++);
        newPattern.erase(pos+2,tmpPos-pos-2);
        if(pos>0&&(newPattern[pos-1]!='/'||(pos+2<newPattern.size()&&newPattern[pos+2]!='/'))){
            string tmpPattern=newPattern;
            tmpPattern.erase(pos,1);
            adjustPattern(tmpPattern,patterns);
        }
        if(pos==0){
            newPattern.insert(pos,"/");
            pos+=1;
        }else if(newPattern[pos-1]!='/'){
            newPattern.insert(pos,"*/");
            pos+=2;
        }
        if(pos+2==newPattern.size() || newPattern[pos+2]!='/'){
            newPattern.insert(pos+2,"/*");
            pos+=2;
        }
        pos+=2;
    };
    if(!StringUtils::Contain(patterns,newPattern)){
        patterns.push_back(newPattern);
    }
}
void globUtil::myglob2(const string &pattern, vector<std::string> &fileNames) {
    vector<string> newPatterns;
    adjustPattern(pattern,newPatterns);
    for(auto newPattern: newPatterns){
        globHandler(newPattern, fileNames);
    }
}

void globUtil::globHandler(const string &pattern, vector<string> &fileNames) {
    int i=0;
    int basePath=0;
    int len=pattern.size();
    bool isRecursive=false;
    // bool isRaw=false;
    bool isFindDir=StringUtils::EndWith(pattern,"/");
    string tmpPattern=pattern;
    tmpPattern.append("\0");
    for(i=0;i<len;i++){
        if(tmpPattern[i]=='*'&&tmpPattern[i+1]=='*'){
            isRecursive=true;
            break;
        }else if(tmpPattern[i]=='/'){
            basePath=i;
        }
    }
    vector<string> tmpFileNames;
    if(!isRecursive){
        myglob1(pattern,tmpFileNames);
        for(auto fileName: tmpFileNames){
            if(StringUtils::EndWith(fileName,"/")==isFindDir && !StringUtils::Contain(fileNames,fileName,true)){
                fileNames.push_back(fileName);
            }
        }
        return;
    }
    string basePattern=tmpPattern.substr(0,basePath+1);
    tmpPattern=tmpPattern.substr(basePath+4,len-(basePath+4));
    myglob1(basePattern,tmpFileNames);
    for(auto fileName: tmpFileNames){
        if(StringUtils::EndWith(fileName,"/")){
            string newPattern=fileName+tmpPattern;
            globHandler(newPattern,fileNames);
            newPattern=fileName+"*/**/"+tmpPattern;
            globHandler(newPattern,fileNames);
        }
    }
}
/*
bool globUtil::wildMatch(const string &pattern, const string &filename){
    return fnmatch(pattern.c_str(),filename.c_str(),FNM_NOESCAPE)==0;
}
*/

}
#endif