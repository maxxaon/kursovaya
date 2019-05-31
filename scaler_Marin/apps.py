import json


class AppInfo:
    def __init__(self, scale_capability, vert_probability,
                 acceptable_exec_time, min_resources):
        self.scale_capability = scale_capability
        self.vert_probability = vert_probability
        self.acceptable_exec_time = acceptable_exec_time
        self.min_resources = min_resources


class Apps:
    def __init__(self, file_name):
        self.app_name2info = {}
        with open(file_name) as input_file:
            data = json.load(input_file)
            for app in data:
                app_name = app['app_name']
                acceptable_exec_time = app['acceptable_exec_time']
                min_resources = app['min_resources']
                scale_capability = app['scale_capability']
                points_for = {}
                for scale, info in app['additional_info'].items():
                    for name, value in info.items():
                        points_for[scale] = points_for.get(scale, 0) + value
                vert_probability = points_for['for_vertical'] / \
                                  (points_for['for_vertical'] + points_for['for_horizontal'])
                self.app_name2info[app_name] = AppInfo(
                    scale_capability, vert_probability,
                    acceptable_exec_time, min_resources
                )

    def get_app_info(self, app_name):
        return self.app_name2info[app_name]

