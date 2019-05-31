import subprocess
from glob import glob


class ResourceAllocator:
    def __init__(self):
        self.current_id = 0
        self.current_tenants = ""
        self.templates = {}
        for template_file_name in glob('./templates/*'):
            key = template_file_name.split('/')[-1].split('.')[0]
            with open(template_file_name) as template_file:
                self.templates[key] = template_file.read()

    def make_test_file(self):
        content = self.templates['structure'].format(tenants=self.current_tenants)
        with open('../vm_allocator_Chupachin/tests/test.dcxml', 'w') as test_file:
            test_file.write(content)

    def create_vm(self, cpu, ram):
        self.current_id += 1
        self.current_tenants += self.templates['create_tenant'].format(
            id=self.current_id, cpu=cpu, ram=ram)
        self.make_test_file()
        proc = subprocess.Popen(["../vm_allocator_Chupachin/build/algo "
                                 "../vm_allocator_Chupachin/tests/test.dcxml"],
                                shell=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        output = proc.stdout.read().decode()
        if 'vm created!!!' in output:
            return self.current_id
        else:
            return None

    def delete_vm(self, vm_id):
        self.current_tenants += self.templates['delete_tenant'].format(id=vm_id)
