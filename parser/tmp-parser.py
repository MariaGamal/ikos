# import re
# import os


# class Parser:
#     def __init__(self, file_name: str) -> None:
#         cwd = os.getcwd()
#         file_path = os.path.join(cwd, file_name)
#         self.file = self.read_file(file_path)

#     @staticmethod
#     def read_file(file_name: str) -> str:
#         with open(file_name) as f:
#             return f.read()

#     def func_names(self) -> list:
#         pattern = r"define\s+\w+\s+@\w+"
#         matches = re.findall(pattern, self.file)
#         return [match.split()[2][1:] for match in matches]

#     def all_iterators(self) -> list:
#         iterator_pattern = r"si32\s+%\.\d+\s+=\s+\d+"
#         iterator_matches = re.findall(iterator_pattern, self.file)
#         return [
#             (match.split()[1], match.split()[3])
#             for match in iterator_matches
#         ]

#     def predecessor_successor(self, matches: list) -> str | None:
#         ret = []

#         for match in matches:
#             predecessor = match.split()[1].split("=")[1][1:-1]
#             successor = match.split()[2].split("=")[1][1:-1]
#             if predecessor == successor:
#                 ret.append(match)

#         return ret or None

#     def loop_iterator(self, block: str) -> str | None:
#         for iterator in self.all_iterators():
#             if iterator[0] in block:
#                 return iterator

#     def is_normal_loop(self, iterator: str, block: str) -> bool:
#         return bool(re.search(iterator + r"\s+silt\s+\d+", block))

#     def is_unrolled_loop(self, iterator: str, block: str) -> bool:
#         division_pattern = r"si32\s+%.\d+\s+=\s+" + iterator + r"\s+sadd.nw\s+\d+"
#         division_matches = re.findall(division_pattern, block)

#         increment_var_pattern = r"\s+" + iterator + r"\s+=\s+(.\d+?)(?=\n)"
#         increment_var = re.findall(increment_var_pattern, block)[0]
#         increment_val_pattern = r"si32\s+" + increment_var + \
#             r"\s+=\s+" + iterator + r"\s+sadd\.nw\s+(.+?)(?=\n)"
#         increment_val = re.findall(increment_val_pattern, block)[0]

#         return len(division_matches) >= 2 and increment_val == "2"

#     def valid_loop(self):
#         ret = []

#         pattern = r"#\d+\s+predecessors={#\d+}\s+successors={#\d+}"
#         matches = re.findall(pattern, self.file)

#         if equals := self.predecessor_successor(matches):
#             for each in equals:
#                 block_pattern = each + r"[\s\S]*?}"
#                 block = re.findall(block_pattern, self.file)[0]
#                 if loop_iterator := self.loop_iterator(block):
#                     if self.is_normal_loop(
#                         loop_iterator[0], block
#                     ) and self.is_unrolled_loop(
#                         loop_iterator[0], block
#                     ):
#                         ret.append(each)

#         return ret


# if __name__ == "__main__":
#     parser = Parser("parser/test/ar-sample")
#     # print(parser.func_names())
#     # print(parser.all_iterators())
#     print(parser.valid_loop())


import re

block1 = """si32 %11 = %.02 sadd.nw 2
si64 %12 = sext %11
si32* %13 = ptrshift $1, 40 * 0, 4 * %12
si32 %14 = load %13, align 4
si64 %15 = sext %.02
si32* %16 = ptrshift $1, 40 * 0, 4 * %15
store %16, %14, align 4"""

block2 = """si32 %18 = %17 sadd.nw 2
si64 %19 = sext %18
si32* %20 = ptrshift $1, 40 * 0, 4 * %19
si32 %21 = load %20, align 4
si64 %23 = sext %22
si32* %24 = ptrshift $1, 40 * 0, 4 * %23
store %24, %21, align 4"""


def are_identical(block1: str, block2: str) -> bool:
    block1 = block1.splitlines()
    block2 = block2.splitlines()
    d = {}
    if len(block1) != len(block2):
        return False
    for block1_line, block2_line in zip(block1, block2):
        x = block1_line.split()
        y = block2_line.split()
        if len(x) != len(y):
            return False
        for x_val, y_val in zip(x, y):
            if x_val[0] == "%" and y_val[0] == "%":
                if x_val[1:] in d and d[x_val[1:]] != y_val[1:]:
                    print(x_val, y_val)
                    return False
                else:
                    d[x_val[1:]] = y_val[1:]
            elif x_val != y_val:
                return False
    return True


print(are_identical(block1, block2))
