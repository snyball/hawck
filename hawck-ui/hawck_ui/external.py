from subprocess import Popen, PIPE

class Exe:
    def __init__(self, name):
        self.name = name
        p = Popen(f"which '{name}'", shell=True, stdout=PIPE)
        if p.wait() != 0:
            raise NameError(f"Unable to find program: {name}")
        self.path = p.stdout.read().strip().decode("utf-8")

    def pathTo(self):
        return self.path
        
