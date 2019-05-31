from sortedcontainers import SortedSet
from request import Request
from response import Response


class Timeline:
    def __init__(self, requests_file_name):
        self.timeline = SortedSet(key=lambda x: x.finish_time if type(x) is Response else x.created_time)
        with open(requests_file_name) as requests_input_file:
            for line in requests_input_file:
                tokens = line.split()
                time, app_name = int(tokens[0]), tokens[1]
                self.add(Request(app_name, time))

    def get_next(self):
        return self.timeline.pop(0) if len(self.timeline) else None

    def add(self, obj):
        self.timeline.add(obj)

    def iterate(self):
        while True:
            next_elem = self.get_next()
            if next_elem:
                yield next_elem
            else:
                break
