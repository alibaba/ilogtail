changeTime()
{
  filePath=$1
  newFileName=$2
  currentTime=`date '+%Y-%m-%d %H:%M:%S'`  
  timeLine=`grep -n "DateTime" $filePath|awk -F: '{print $1}' `
  timeContent="\"DateTime\": \"$currentTime\","
  sed "${timeLine}c $timeContent" ${filePath}>$newFileName
  return 0
}
changeTime "$1" "$2"