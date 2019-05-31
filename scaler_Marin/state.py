from queue import Queue
from instance import Instance


class State:
    def __init__(self, balancer, resource_allocator):
        self.app_name2instances = {}
        self.app_name2queue = {}
        self.balancer = balancer
        self.resource_allocator = resource_allocator

    def get_free_instances(self, app_name):
        yield from self.balancer.get_balanced(self.app_name2instances.get(app_name, []))

    def get_all_instances(self, app_name):
        return self.app_name2instances.get(app_name, [])

    def get_from_queue(self, app_name):
        if self.app_name2queue.get(app_name, Queue()).empty():
            return None
        else:
            return self.app_name2queue.get(app_name, Queue()).get()

    def add_to_queue(self, request):
        self.app_name2queue.setdefault(request.app_name, Queue()).put(request)

    def add_instance(self, app_name, cpu, ram):
        vm_id = self.resource_allocator.create_vm(cpu, ram)
        if vm_id is None:
            return False
        self.app_name2instances.setdefault(app_name, []).append(Instance(vm_id, cpu, ram))
        return True

    def get_max_instance(self, app_name):
        best_instance = None
        for instance in self.get_all_instances(app_name):
            if best_instance is None or \
                    instance.cpu + instance.ram > best_instance.cpu + best_instance.ram:
                best_instance = instance
        return best_instance

    def get_min_instance(self, app_name):
        best_instance = None
        for instance in self.get_all_instances(app_name):
            if best_instance is None or \
                    instance.cpu + instance.ram < best_instance.cpu + best_instance.ram:
                best_instance = instance
        return best_instance

    def remove_instance(self, app_name, instance):
        for i in range(len(self.app_name2instances[app_name])):
            if self.app_name2instances[app_name][i] is instance:
                self.app_name2instances[app_name].pop(i)
                break


