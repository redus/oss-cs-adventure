import re

# using O(1) key lookup for rare words
count_dict = dict()

counts = open('gene.counts', 'r')
for line in counts:
	words = line.split()
	count = int(words[0])

	if words[1] == 'WORDTAG':
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
		word = words[0]
		rare_type = '_RARE_'

		if re.search('[0-9]', word):
			rare_type = '_NUM_'
		elif word.isalpha() and word.isupper():
			rare_type = '_ALL_CAPS_'
		elif word[-1].isupper():
			rare_type = '_LAST_CAPS_'
		line = '%s %s\n' % (rare_type, words[1])
	train_rare.write(line)

train_original.close()
train_rare.close()