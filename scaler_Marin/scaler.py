import random


class Scaler:
    def __init__(self, resource_allocator):
        self.resource_allocator = resource_allocator
        self.memory_balance = 0.5

    def decrease_balance(self):
        self.memory_balance *= 0.9

    def increase_balance(self):
        self.memory_balance = 1 - (1 - self.memory_balance) * 0.9

    def scale_up(self, cur_time, state, apps, app_name):
        app_info = apps.get_app_info(app_name)
        vert_probability = app_info.vert_probability * self.memory_balance / \
                           (app_info.vert_probability * self.memory_balance +
                            (1 - app_info.vert_probability) * (1 - self.memory_balance))
        if random.uniform(0, 1) < vert_probability:
            instance = state.get_min_instance(app_name)
            self.resource_allocator.delete_vm(instance.vm_id)
            vm_id = self.resource_allocator.create_vm(instance.cpu + max(1, int(0.2 * instance.cpu)),
                                                      instance.ram + max(1, int(0.2 * instance.ram)))
            if vm_id:
                instance.cpu += max(1, int(0.2 * instance.cpu))
                instance.ram += max(1, int(0.2 * instance.ram))
                instance.vm_id = vm_id
                if not instance.is_free():
                    instance.recalc(cur_time)
                self.increase_balance()
                return instance
            else:
                self.decrease_balance()
                return None
        else:
            resp = state.add_instance(app_name, app_info.min_resources['cpu'],
                                      app_info.min_resources['ram'])
            if resp:
                self.decrease_balance()
            else:
                self.increase_balance()
            return None

    def scale_down(self, cur_time, state, apps, app_name):
        app_info = apps.get_app_info(app_name)
        vert_probability = 1 - app_info.vert_probability * self.memory_balance / \
                           (app_info.vert_probability * self.memory_balance +
                            (1 - app_info.vert_probability) * (1 - self.memory_balance))
        free_instances = list(state.get_free_instances(app_name))
        if random.uniform(0, 1) < vert_probability or \
                len(free_instances) == 0 or len(state.get_all_instances(app_name)) == 1:
            instance = state.get_max_instance(app_name)
            self.resource_allocator.delete_vm(instance.vm_id)
            instance.cpu -= min(instance.cpu - 1, max(int(0.2 * instance.cpu), 1))
            instance.ram -= min(instance.ram - 1, max(int(0.2 * instance.ram), 1))
            instance.vm_id = self.resource_allocator.create_vm(instance.cpu, instance.ram)
            if not instance.is_free():
                instance.recalc(cur_time)
        else:
            instance = free_instances[0]
            self.resource_allocator.delete_vm(instance.vm_id)
            state.remove_instance(app_name, instance)
