

# using O(1) key lookup for rare words
# values dont' matter, putting boolean
rare_dict = dict()

counts = open('gene.counts', 'r')
for line in counts:
	words = line.split()
	if int(words[0]) < 5 and words[1] == "WORDTAG":
		key = '%s_%s' %(words[-1], words[-2])
		rare_dict[key] = True
counts.close()


train_rare = open('gene.rare.train', 'w')
train_original = open('gene.train', 'r')

for line in train_original:
	words = line.split()
	key = '_'.join(words)
	if words and key in rare_dict:
		line = '_RARE_ %s\n' % words[1]
	train_rare.write(line)


train_original.close()
train_rare.close()