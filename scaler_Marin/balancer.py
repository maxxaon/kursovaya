
def default_comparator(inst):
    if inst.is_free():
        return float('inf')
    else:
        return inst.exec_time()


class Balancer:
    def __init__(self, comparator=default_comparator):
        self.comparator = comparator

    def get_balanced(self, instances):
        instances.sort(key=self.comparator)
        for instance in instances:
            if instance.is_free():
                yield instance
            else:
                break
