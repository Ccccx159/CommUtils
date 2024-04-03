import json, os
import subprocess
import re
def get_compiler_version(compiler):
    result = subprocess.run([compiler, '--version'], stdout=subprocess.PIPE)
    version_info = result.stdout.decode('utf-8').strip()
    version_match = re.search(r"(\d+\.\d+\.\d+)", version_info)
    if version_match:
        version = version_match.group(1)
        major, minor, patchlevel = version.split('.')
        return major, minor, patchlevel
    else:
        return None, None, None

def insert_version_in_command(command, major, minor, patchlevel):
    pattern = re.compile(r'-D\w+')
    match = pattern.search(command)
    if match:
        index = match.start()
        new_command = command[:index] + f'-D__GNUC__={major} -D__GNUC_MINOR__={minor} -D__GNUC_PATCHLEVEL__={patchlevel} ' + command[index:]
        return new_command
    return command

def main(compile_commands):
    with open(compile_commands, 'r') as f:
        data = json.load(f)
        for item in data:
            command = item['command']
            compiler = command.split()[0]
            major, minor, patchlevel = get_compiler_version(compiler)
            new_command = insert_version_in_command(command, major, minor, patchlevel)
            item['command'] = new_command

    with open(compile_commands, 'w') as f:
        json.dump(data, f, indent=2)

if __name__ == "__main__":
    command_file = os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + '/..') + '/build/compile_commands.json'
    main(command_file)