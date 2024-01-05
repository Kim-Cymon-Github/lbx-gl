if not exist lib\lbx\include\gl mkdir lib\lbx\include\gl
copy src\*.h lib\lbx\include\gl
pushd lib\lbx
git add include/gl/*
popd
