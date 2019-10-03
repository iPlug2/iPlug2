echo CLEAR AU CACHE SCRIPT
if [ -f "$HOME/Library/Application Support/AU Lab/com.apple.audio.aulab_componentcache.plist" ]
  then
	echo deleting "$HOME/Library/Application Support/AU Lab/com.apple.audio.aulab_componentcache.plist"
	rm "$HOME/Library/Application Support/AU Lab/com.apple.audio.aulab_componentcache.plist"
fi

if [ -f "$HOME/Library/Caches/AudioUnitCache/com.apple.audiounits.cache" ]
  then
	echo deleting "$HOME/Library/Caches/AudioUnitCache/com.apple.audiounits.cache"
	rm "$HOME/Library/Caches/AudioUnitCache/com.apple.audiounits.cache"
fi

if [ -f "$HOME/Library/Caches/AudioUnitCache/com.apple.audiounits.sandboxed.cache" ]
then
echo deleting "$HOME/Library/Caches/AudioUnitCache/com.apple.audiounits.sandboxed.cache"
rm "$HOME/Library/Caches/AudioUnitCache/com.apple.audiounits.sandboxed.cache"
fi


pgrep -x AudioComponentRegistrar >/dev/null && killall -9 AudioComponentRegistrar; echo "killed AudioComponentRegistrar" || echo "AudioComponentRegistrar Process not found"
