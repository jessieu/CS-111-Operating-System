#!/usr/bin/python
import sys

ref_block_bitmap = {}
ref_inodes = {}
parent_nodes = {}

exit_code = 0

class Superblock:
	def __init__(self, sb):
		self.total_blocks = int(sb[1])
		self.total_inodes = int(sb[2])
		self.fst_inode = int(sb[7])
class Bfree:
	def __init__(self):
		self.free_blocks = []
	def add_blocks(self, block_num):
		self.free_blocks.append(int(block_num))

class Ifree:
	def __init__(self):
		self.free_inodes = []
	def add_inodes(self, inode_num):
		self.free_inodes.append(int(inode_num))

class Inode:
	def __init__(self, ino):
		self.inumber = int(ino[1])
		self.f_type = ord(ino[2])
		self.link_count = int(ino[6])
		self.direct = []
		for i in range(12, 24):
			self.direct.append(int(ino[i]))
		self.s_indirect = int(ino[24])
		self.d_indirect = int(ino[25])
		self.t_indirect = int(ino[26])

class Dirent:
	def __init__(self, dirent):
		self.parent = int(dirent[1])
		self.rf_inode = int(dirent[3])
		self.name = dirent[6]

class Indirect:
	def __init__(self, ino):
		self.owner = int(ino[1])
		self.level = int(ino[2])
		self.logical_offset = int(ino[3])
		self.indirect_block_num = int(ino[4])
		self.rf_block = int(ino[5])

def block_check(superblock, bfree, inode_num, block_num, block_type, offset):
	''' Helper function for all blocks check - direct and indirect '''
	if block_num != 0:
		# block number not in range
		if block_num < 0 or block_num > superblock.total_blocks - 1:
			print("INVALID {} {} IN INODE {} AT OFFSET {}".format(block_type, block_num, inode_num, offset))
			exit_code = 2
		# use reserved block
		elif block_num < 8: # other than those metadata, group descriptor also has 3 blocks reserved
			print("RESERVED {} {} IN INODE {} AT OFFSET {}".format(block_type, block_num, inode_num, offset))
			exit_code = 2
		elif block_num in ref_block_bitmap:
			ref_block_bitmap[block_num].append((block_type, block_num, inode_num, offset))
		else:
			ref_block_bitmap[block_num] = [(block_type, block_num, inode_num, offset)]

def inode_block_check(superblock, bfree, inodes):
	''' Block Pointer in Inode Consistency Aduit '''
	for ino in inodes:
		offset = 0
		for block_num in ino.direct:
			if block_num != 0:
				block_check(superblock, bfree, ino.inumber, block_num, "BLOCK", offset)
			offset += 1
		if ino.s_indirect != 0:
			offset = 12
			block_check(superblock, bfree, ino.inumber, ino.s_indirect, "INDIRECT BLOCK", offset)
		if ino.d_indirect != 0:
			offset = 12 + 1024/4 # 1024 is the block size
			block_check(superblock, bfree, ino.inumber, ino.d_indirect, "DOUBLE INDIRECT BLOCK", offset)
		if ino.t_indirect != 0:
			offset = 12 + 1024/4 + (1024/4)**2
			block_check(superblock, bfree, ino.inumber, ino.t_indirect, "TRIPLE INDIRECT BLOCK", offset)
		# if ino.f_type == 'd':
		# 	allocated_inodes[ino.inumber] = ino.link_count

def indirect_block_check(superblock, bfree, indirects):
	''' Indirect Block Consistency Audit'''
	for indirect in indirects:
		if indirect.rf_block != 0:
			if indirect.level == 1:
				block_check(superblock, bfree, indirect.owner, indirect.rf_block, "INDIRECT BLOCK", indirect.logical_offset)
			elif indirect.level == 2:
				block_check(superblock, bfree, indirect.owner, indirect.rf_block, "DOUBLE INDIRECT BLOCK", indirect.logical_offset)
			elif indirect.level == 3:
				block_check(superblock, bfree, indirect.owner, indirect.rf_block, "TRIPLE INDIRECT BLOCK", indirect.logical_offset)


def data_block_check(superblock, bfree):
	''' Data Block Consistency Audit'''
	for block_num in range(8, superblock.total_blocks):
		# claimed as free but actually not
		if block_num in bfree.free_blocks and block_num in ref_block_bitmap:
		   	print("ALLOCATED BLOCK {} ON FREELIST".format(block_num))
		   	exit_code = 2
		# neither referenced by file nor being free
		if block_num not in bfree.free_blocks and block_num not in ref_block_bitmap:
		   print("UNREFERENCED BLOCK {}".format(block_num))
		   exit_code = 2
		# duplicate blocks
		if block_num in ref_block_bitmap and len(ref_block_bitmap[block_num]) > 1:
			for dp_block in ref_block_bitmap[block_num]:
				print("DUPLICATE {} {} IN INODE {} AT OFFSET {}".format(dp_block[0], dp_block[1], dp_block[2], dp_block[3]))
				exit_code = 2


def directory_check(superblock, ifree, dirents):
	''' Directory Consistency Audit '''
	for dirent in dirents:
		# Don't confuse yourself:
		# rf_inode is the file entries the dirent links to
		# parent is the inode number of dirent
		if dirent.rf_inode < 1 or dirent.rf_inode > superblock.total_inodes - 1:
			print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(dirent.parent, dirent.name, dirent.rf_inode))
			exit_code = 2
		elif dirent.rf_inode != 2:
			if dirent.rf_inode in ifree.free_inodes:
				print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(dirent.parent, dirent.name, dirent.rf_inode))
				exit_code = 2

		if dirent.rf_inode in ref_inodes:
			ref_inodes[dirent.rf_inode] += 1
		else:
			ref_inodes[dirent.rf_inode] = 1
		# match parent with child - use to check the corretness of ..
		if dirent.rf_inode == 2:
			parent_nodes[dirent.rf_inode] =2
		else:
			parent_nodes[dirent.rf_inode] = dirent.parent


def inode_allocation_check(superblock, ifree, inodes):
	''' Inode Allocation and Link Count Audit '''
	allocated_inodes = []
	for ino in inodes:
		if ino.f_type != 0: # Non-zero type inode is allocated
			if ino.inumber in ifree.free_inodes:
				print("ALLOCATED INODE {} ON FREELIST".format(ino.inumber))
				exit_code = 2
			allocated_inodes.append(ino.inumber)

		if ino.inumber in ref_inodes:
			if ref_inodes[ino.inumber] != ino.link_count:
				print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(ino.inumber, ref_inodes[ino.inumber] , ino.link_count))
				exit_code = 2
		else:
			if ino.link_count != 0:
				print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(ino.inumber, 0, ino.link_count))
				exit_code = 2

	for inumber in range(superblock.fst_inode, superblock.total_inodes):
		if inumber not in allocated_inodes and inumber not in ifree.free_inodes:
			print("UNALLOCATED INODE {} NOT ON FREELIST".format(inumber))
			exit_code = 2


def ref_link_check(superblock, ifree, dirents):
	''' Dot and Dot Dot Link Consistency Audit'''
	for dirent in dirents:
		# "." is the first entry and it refers to itself
		if dirent.name == "'.'" and dirent.rf_inode != dirent.parent:
			print("DIRECTORY INODE {} NAME '.' LINK TO INODE {} SHOULD BE {}".format(dirent.parent, dirent.rf_inode, dirent.parent))
			exit_code = 2
		# ".." is the second entry
		# tricky - dot dot links to parent directory(have stored in directory check)
		# inode 2 is for root directory
		elif dirent.name == "'..'" and dirent.rf_inode != 2 and dirent.rf_inode != parent_nodes[dirent.parent]:
			print("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(dirent.parent, dirent.rf_inode, parent_nodes[dirent.parent]))
			exit_code = 2

def main():
	superblock = None
	bfree = Bfree()	# list of free blocks
	ifree = Ifree() # list of free inodes
	inodes = [] # list of inode object
	dirents = [] # list of directory entry
	indirects = [] # list of indirect blocks

	if (len(sys.argv) is not 2):
		sys.stderr.write("Too many arguments\n")
		sys.exit(1)

	try:
		file_name = sys.argv[1]
	except:
		sys.stderr.write("Bad Argument\n")
		sys.exit(1)
	# read from file
	try:
		with open(file_name, 'r') as f:
			for data_line in f:
				line = data_line.rstrip().split(',')

				if line[0] == "SUPERBLOCK":
				   	superblock = Superblock(line)
				elif line[0] == "BFREE":
					bfree.add_blocks(line[1])
				elif line[0] == "IFREE":
					ifree.add_inodes(line[1])
				elif line[0] == "INODE":
					inodes.append(Inode(line))
				elif line[0] == "DIRENT":
					dirents.append(Dirent(line))
				elif line[0] == "INDIRECT":
					indirects.append(Indirect(line))
	except:
		sys.stderr.write("File Procrocessing Error\n")
		sys.exit(1)

	inode_block_check(superblock, bfree, inodes)
	indirect_block_check(superblock, bfree, indirects)
	data_block_check(superblock, bfree)

	directory_check(superblock, ifree, dirents)
	inode_allocation_check(superblock, ifree, inodes)
	ref_link_check(superblock, ifree, dirents)


if __name__ == "__main__":
	main()
	exit(exit_code)
