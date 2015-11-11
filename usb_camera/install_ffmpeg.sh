#!/bin/sh

# If the download doesn't work, the release link may have changed.
# Check http://johnvansickle.com/ffmpeg/, copy the address of the
# latest release, and replace in the shell script.

# CHANGE THIS TO THE APPROPRIATE FILENAME
FNAME=ffmpeg-release-32bit-static.tar.xz;

echo "Creating $HOME/bin directory if it doesn't exist...";
mkdir -p $HOME/bin;

echo "Removing old versions of ffmpeg...";
rm -rf $HOME/bin/ffmpeg*;

echo "Downloading ffmpeg..."
wget -P $HOME/bin http://johnvansickle.com/ffmpeg/releases/$FNAME;

echo "Unpacking...";
tar -xf $HOME/bin/$FNAME -C $HOME/bin;

echo "Cleaning up...";
rm $HOME/bin/$FNAME;

mv $HOME/bin/ffmpeg*/ $HOME/bin/ffmpeg;

echo "Create a $HOME/.bashrc file and the following line \"export PATH=\$HOME/bin/ffmpeg:\$PATH\"";
