import re
import os


# instructions:
#   - check if for loop
#   - if for loop, check if it's a normal for loop
#   - if normal for loop, check if it's unrolled
#   - if unrolled, find the iterator variable


class Parser:
    def __init__(self, file_name: str) -> None:
        cwd = os.getcwd()
        file_path = os.path.join(cwd, file_name)
        self.file = self.read_file(file_path)

    @staticmethod
    def read_file(file_name: str) -> str:
        with open(file_name) as f:
            return f.read()

    def func_name(self) -> list:
        pattern = r"define\s+\w+\s+@\w+"
        matches = re.findall(pattern, self.file)
        return [match.split()[2][1:] for match in matches]

    def loop_iterator(self) -> list:
        iterator_pattern = r"si32\s+%\.\d+\s+=\s+\d+"
        iterator_value_pattern = r"si32\s+%\.\d+\s+=\s+\d+"
        iterator_matches = re.findall(iterator_pattern, self.file)
        iterator_values = re.findall(iterator_value_pattern, self.file)
        return [
            f"{match.split()[1]} = {value.split()[3]}"
            for match, value in zip(iterator_matches, iterator_values)
        ]


if __name__ == "__main__":
    parser = Parser("parser/test/ar-sample")
    print(parser.func_name())
    print(parser.loop_iterator())
