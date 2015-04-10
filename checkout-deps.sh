checkout ()
{
    rm -rf $name
    git clone $repo -b $branch $name
    if [ -n "$origin" ]; then
      cd $name
      git remote rm origin
      git remote add origin $origin
      cd ..
    fi
}

name=ThreadLib
branch=master
repo="https://github.com/littleku/ThreadLib"
origin=
checkout