import os

threads = [1, 2, 4, 8]
iterations = [1, 10, 100, 1000, 10000, 100000]
sync_opt = ['m', 's', 'c']
yield_opt = ['i', 'd', 'l', 'id', 'il', 'dl', 'idl']
sync_list = ['m', 's']

f = open ("lab2_add.csv", "w+")

# threads and iterations without yield
# for i in threads:
#     for j in iterations:
#         cmd = './lab2_add --threads={} --iterations={}'.format(i, j)
#         f.write(os.popen(cmd).read())
#
# # threads and iterations with yield
# for i in threads:
#     for j in iterations:
#         cmd = './lab2_add --threads={} --iterations={} --yield'.format(i, j)
#         f.write(os.popen(cmd).read())
#
# # threads and iteration with sync_opt and without yield
# for i in threads:
#     for j in iterations:
#         for k in sync_opt:
#             cmd = './lab2_add --threads={} --iterations={} --sync={}'.format(i, j, k)
#             f.write(os.popen(cmd).read())

# threads and iterations with sync and yieled
for i in threads:
    for j in iterations:
        for k in sync_opt:
            cmd = './lab2_add --threads={} --iterations={} --sync={} --yield'.format(i, j, k)
            os.system(cmd)
            # f.write(os.popen(cmd).read())

f.close()

# f = open("lab2_list.csv", "w+")
#
# # threads and iterations without yield
# for i in threads:
#     for j in iterations:
#         cmd = './lab2_list --threads={} --iterations={}'.format(i, j)
#         f.write(os.popen(cmd).read())
#
# # threads and iterations with yield
# for i in threads:
#     for j in iterations:
#         for k in yield_opt:
#             cmd = './lab2_list --threads={} --iterations={} --yield={}'.format(i, j, k)
#             f.write(os.popen(cmd).read())
#
# # threads and iteration with sync_opt and without yield
# for i in threads:
#     for j in iterations:
#         for k in sync_list:
#             cmd = './lab2_list --threads={} --iterations={} --sync={}'.format(i, j, k)
#             f.write(os.popen(cmd).read())
#
# # threads and iterations with sync and yiled
# for i in threads:
#     for j in iterations:
#         for k in sync_list:
#             for l in yield_opt:
#                 cmd = './lab2_list --threads={} --iterations={} --sync={} --yield={}'.format(i, j, k, l)
#                 f.write(os.popen(cmd).read())
