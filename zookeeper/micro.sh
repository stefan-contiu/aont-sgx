# 128 MB File, 0-3 SE blocks, 4 block sizes

for se in 0 1 2 3
do
	for block_size in 256 1024 2048 4096
	do
		../client/cli-aont.o -micro 131072 $block_size $se
		cd ../server && ./aont_srv
	done
done
