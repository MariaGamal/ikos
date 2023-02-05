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
        iterator_matches = re.findall(iterator_pattern, self.file)
        return [
            f"{match.split()[1]} = {match.split()[3]}"
            for match in iterator_matches
        ]

    def is_template(self) -> bool:
        '''
        - check if the predecessor of the block is the same as the successor
        - check if first statment is in the form "loop iterator silt n"
        - check if the loop iterator is incremented by 2 
        '''
        pattern = "#\d+\s+predecessors={#\d+}\s+successors={#\d+}"
        matches = re.findall(pattern, self.file)
        for match in matches:
            predecessor = match.split()[1].split("=")[1][1:-1]
            successor = match.split()[2].split("=")[1][1:-1]
            if predecessor == successor:
                blocks = self.file.split(match)[1]
                block = re.findall(r"{[\s\S]*?}", blocks)[0]
                loop_iterator = self.loop_iterator()[0].split()[0]
                if re.search(loop_iterator + r"\s+silt\s+\d+", block):
                    additions = re.findall(r"si32\s+%\d+\s+=\s+" + loop_iterator + r"\s+sadd\.nw\s+2", block)
                    destination = re.search(r"si32\s+" + loop_iterator+ r"\s+=\s+%\d+", block).group().split('=')[1].strip()
                    for addition in additions:
                        assignment_register = addition.split('=')[0].strip()[5:]
                        if assignment_register == destination:
                            return True
        return False

    def is_unrolled(self) -> bool:
        '''
        get the two divisions of statement and check if they are the same
        '''
        pass 



if __name__ == "__main__":
    parser = Parser("ar-sample")
    print(parser.func_name())
    print(parser.loop_iterator())
    print(parser.is_template())
