
class Instance:
    def __init__(self, vm_id, cpu, ram):
        self.vm_id = vm_id
        self.ram = cpu
        self.cpu = ram
        self.free = True
        self.remaining_part = 1
        self.finish_time = 0
        self.start_time = 0

    def exec_time(self):
        return self.remaining_part * 100 / (self.ram * self.cpu)

    def add_request(self, cur_time):
        self.start_time = cur_time
        self.finish_time = cur_time + self.exec_time()
        self.free = False

    def is_free(self):
        return self.free

    def make_free(self):
        self.free = True
        self.remaining_part = 1

    def recalc(self, cur_time):
        execed_part = self.remaining_part * (cur_time - self.start_time) / \
                      (self.finish_time - self.start_time)
        self.remaining_part -= execed_part
        self.start_time = cur_time
        self.finish_time = self.start_time + self.exec_time()

