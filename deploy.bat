chcp 65001
if not exist lib\lbx\include\gl mkdir lib\lbx\include\gl
attrib -r /s /d lib\lbx\include\gl
copy /Y src\*.h lib\lbx\include\gl
pushd lib\lbx
git add include/gl/*
git add lib/*
git commit -m "배포 업데이트"
git push
popd