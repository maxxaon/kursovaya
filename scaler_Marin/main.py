from timeline import Timeline
from request import Request
from balancer import Balancer
from state import State
from response import Response
from scaler import Scaler
from apps import Apps
from resource_allocator import ResourceAllocator


def pull_queue(state, app_name, cur_time, timeline, apps_info):
    if len(state.get_all_instances(app_name)) == 0:
        app_info = apps_info.get_app_info(app_name)
        state.add_instance(app_name, app_info.min_resources['cpu'], app_info.min_resources['ram'])
    for instance in state.get_free_instances(app_name):
        request = state.get_from_queue(app_name)
        if request:
            instance.add_request(cur_time)
            timeline.add(Response(app_name, request.created_time, cur_time, instance))
        else:
            break


def update_response(timeline, instance):
    for i in range(len(timeline.timeline)):
        it = timeline.timeline[i]
        if it is Response and it.instance is instance:
            response = timeline.timeline.pop(i)
            response.finish_time = instance.finish_time
            timeline.add(response)
            break


def main():
    timeline = Timeline("input/requests.txt")
    resource_allocator = ResourceAllocator()
    state = State(Balancer(), resource_allocator)
    apps_info = Apps("input/apps.json")
    scaler = Scaler(resource_allocator)

    for event in timeline.iterate():
        if type(event) is Request:
            request = event
            cur_time = request.created_time
            state.add_to_queue(request)
        elif type(event) is Response:
            response = event
            cur_time = response.finish_time
            response.instance.make_free()
            exec_time = cur_time - response.req_created_time
            #print(response.instance.cpu * response.instance.ram)
            acceptable_exec_time = apps_info.get_app_info(response.app_name).acceptable_exec_time
            if exec_time > acceptable_exec_time[1]:
                print(f'exec_time of {response.app_name} is too long ({exec_time}), scaling up!')
                instance = scaler.scale_up(cur_time, state, apps_info, response.app_name)
                if instance:
                    update_response(timeline, instance)
            elif exec_time < acceptable_exec_time[0]:
                print(f'exec_time of {response.app_name} is too short ({exec_time}), scaling down!')
                instance = scaler.scale_down(cur_time, state, apps_info, response.app_name)
                if instance:
                    update_response(timeline, instance)
        mb, vp = scaler.memory_balance, apps_info.get_app_info('a').vert_probability
        #print(mb, vp * mb / (mb * vp + (1 - mb) * (1 - vp)), sep='\t')
        pull_queue(state, event.app_name, cur_time, timeline, apps_info)
            

if __name__ == '__main__':
    main()



