import re
import os


class Parser:
    def __init__(self, file_name: str) -> None:
        cwd = os.getcwd()
        file_path = os.path.join(cwd, file_name)
        self.file = self.read_file(file_path)

    @staticmethod
    def read_file(file_name: str) -> str:
        with open(file_name) as f:
            return f.read()

    def func_names(self) -> list:
        pattern = r"define\s+\w+\s+@\w+"
        matches = re.findall(pattern, self.file)
        return [match.split()[2][1:] for match in matches]

    def all_iterators(self) -> list:
        iterator_pattern = r"%\.\d+\s+silt\s+\d+"
        iterator_matches = re.findall(iterator_pattern, self.file)
        return [
            match.split()[0]
            for match in iterator_matches
        ]

    def predecessor_successor(self, matches: list) -> list[str] | None:
        ret = []

        for match in matches:
            predecessor = match.split()[1].split("=")[1][1:-1]
            successor = match.split()[2].split("=")[1][1:-1]
            if predecessor == successor:
                ret.append(match)

        return ret or None

    def loop_iterator(self, block: str) -> str | None:
        for iterator in self.all_iterators():
            if iterator in block:
                return iterator

    def is_normal_loop(self, iterator: str, block: str) -> bool:
        return bool(re.search(iterator + r"\s+silt\s+\d+", block))

    def is_iterator_incremented_twice(self, iterator: str, block: str):
        additions = re.findall(r"si32\s+%\d+\s+=\s+" + iterator + r"\s+sadd\.nw\s+2", block)
        destinations = re.findall(r"si32\s+" + iterator+ r"\s+=\s+%\d+", block)
        if destinations is None: 
            return (False, '')
        for add in additions:
            for dest in destinations:
                assignment_var = add.split()[1].strip()
                dest_var = dest.split('=')[1].strip()
                if assignment_var == dest_var:
                    return (True, add)
        return (False,'')

    def are_identical(self, iterator: str, block1: list[str], block2: list[str]) -> bool:
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
                    if x_val in d and x_val != iterator and d[x_val] != y_val:
                        return False
                    else:
                        d[x_val] = y_val
                elif x_val != y_val:
                    return False
        return True

    def is_unrolled_loop(self, iterator: str, block: str) -> bool:
        incremented, stat = self.is_iterator_incremented_twice(iterator, block)
        if not incremented:
            return False    
        statements = list(map(lambda x: x.strip(), block.splitlines()))
        skip = statements.index(stat)
        statements = statements[2: skip]
        mid = len(statements)//2
        block1, block2 = statements[:mid -1], statements[mid:]
        for stat in block2:
            if re.search(iterator + r"\s+sadd\.nw\s+1", stat):
                block2.remove(stat)
        if not self.are_identical(iterator, block1, block2):
            return False
        return True

    def valid_loop(self):
        ret = []

        pattern = r"#\d+\s+predecessors={#\d+}\s+successors={#\d+}"
        matches = re.findall(pattern, self.file)

        if equals := self.predecessor_successor(matches):
            for each in equals:
                block_pattern = each + r"[\s\S]*?}"
                block = re.findall(block_pattern, self.file)[0]
                if loop_iterator := self.loop_iterator(block):
                    if self.is_normal_loop(
                        loop_iterator, block
                    ) and self.is_unrolled_loop(
                        loop_iterator, block
                    ):
                        ret.append(block)
        return ret

    def get_variables(self):
        pattern = "store\s+%\d+,\s+%\d+[a-z|,\s++\d+]*\\n"
        ret = []
        for loop in self.valid_loop():
            stores = re.findall(pattern, loop)
            if len(stores) != 2:
                return []
            read0, write0 = stores[0].split()[2].split(',')[0], stores[0].split()[1].split(',')[0] 
            read1, write1 = stores[1].split()[2].split(',')[0], stores[1].split()[1].split(',')[0]
            ret.append([(read0, write0), (read1, write1)])
        return ret

if __name__ == "__main__":
    parser = Parser("parser/test/ar-sample")
    # print(parser.func_names())
    # print(parser.all_iterators())
    # print(parser.valid_loop())
    print(parser.get_variables()) # [(%6, %9), (%14, %18)]