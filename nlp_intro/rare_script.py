

# using O(1) key lookup for rare words
count_dict = dict()

counts = open('gene.counts', 'r')
for line in counts:
	words = line.split()
	count = int(words[0])

	if words[1] == "WORDTAG":
		key = words[-1]
		if key in count_dict:
			count_sum = count_dict[key] + count
			count_dict[key] = count_sum
		else:
			count_dict[key] = count
counts.close()

# print count_dict.keys()

train_rare = open('gene.rare.train', 'w')
train_original = open('gene.train', 'r')

for line in train_original:
	words = line.split()

	if words and count_dict[words[0]] < 5:
		line = '_RARE_ %s\n' % words[1]
	train_rare.write(line)


train_original.close()
train_rare.close()