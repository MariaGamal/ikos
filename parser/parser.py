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
        iterator_pattern = r"si32\s+%\.\d+\s+=\s+\d+"
        iterator_matches = re.findall(iterator_pattern, self.file)
        return [
            (match.split()[1], match.split()[3])
            for match in iterator_matches
        ]

    def predecessor_successor(self, matches: list) -> str | None:
        ret = []

        for match in matches:
            predecessor = match.split()[1].split("=")[1][1:-1]
            successor = match.split()[2].split("=")[1][1:-1]
            if predecessor == successor:
                ret.append(match)

        return ret or None

    def loop_iterator(self, block: str) -> str | None:
        for iterator in self.all_iterators():
            if iterator[0] in block:
                return iterator

    def is_normal_loop(self, iterator: str, block: str) -> bool:
        return bool(re.search(iterator + r"\s+silt\s+\d+", block))

    def is_unrolled_loop(self, iterator: str, block: str) -> bool:
        division_pattern = r"si32\s+%.\d+\s+=\s+" + iterator + r"\s+sadd.nw\s+\d+"
        division_matches = re.findall(division_pattern, block)

        increment_var_pattern = r"\s+" + iterator + r"\s+=\s+(.\d+?)(?=\n)"
        increment_var = re.findall(increment_var_pattern, block)[0]
        increment_val_pattern = r"si32\s+" + increment_var + \
            r"\s+=\s+" + iterator + r"\s+sadd\.nw\s+(.+?)(?=\n)"
        increment_val = re.findall(increment_val_pattern, block)[0]

        return len(division_matches) >= 2 and increment_val == "2"

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
                        loop_iterator[0], block
                    ) and self.is_unrolled_loop(
                        loop_iterator[0], block
                    ):
                        ret.append(each)

        return ret


if __name__ == "__main__":
    parser = Parser("parser/test/ar-sample")
    # print(parser.func_names())
    # print(parser.all_iterators())
    print(parser.valid_loop())
