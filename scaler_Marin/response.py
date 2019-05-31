
class Response:
    def __init__(self, app_name, req_created_time, start_time, instance):
        self.app_name = app_name
        self.instance = instance
        self.req_created_time = req_created_time
        self.start_time = start_time
        self.finish_time = instance.finish_time

