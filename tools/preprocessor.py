import os
import pathlib
import re
from shutil import copyfile
import subprocess
import sys
import json

total_lines = 0

def write_file(filename, contents):
	file = open(filename, "w", encoding='utf-8')
	for line in contents:
		file.write(line)
	file.close()

environment_variables = {}
es_definition_headers = []
es_definition_functions = []
max_depth = 0
files_with_custom_depth = set()

game_object_type_enums = []

keywords = ["game_object_definitions"]

command_pattern = r'^[\s]*?// ##(\d{0,5}) (.+)$'

in_place_extensions = [".h.src"]
extensions = [".cc", ".h", ".h.src", ".frag", ".vert"]
glsl_extensions = [".frag", ".vert"]

extension_regex = re.compile(r"\.[\w0-9_.]+$")

snake_case_regex = re.compile(r"(?<!^)(?=[A-Z])")
def to_snake_case(name):
	global snake_case_regex
	return snake_case_regex.sub("_", name).upper()

# headers are always scanned, because they have information that is needed in the preprocessing of other files
def handle_headers(filename, contents):
	global es_definition_headers
	global es_definition_functions
	global game_object_type_enums
	
	read_namespace = False
	for line in contents:
		if "namespace es" in line:
			read_namespace = True
		elif read_namespace:
			if "void define" in line:
				es_definition_headers.append(filename)
				es_definition_functions.append(re.compile(r"^void ([\w]+)").match(line.strip()).group(1))
				read_namespace = False
		
		if match := re.match(command_pattern, line):
			command = match.group(2).strip()
			if "game_object_definitions" in command:
				rest = " ".join(command.split(" ")[1:])
				game_object_type_enums.append(to_snake_case(rest.split(" ")[0]))

def preprocess(filename, contents, depth):
	global total_lines
	global es_definition_headers
	global es_definition_functions
	global max_depth
	global files_with_custom_depth

	if "include" not in filename:
		total_lines = total_lines + len(contents)
	
	new_contents = []
	read_namespace = False
	for line in contents:
		if match := re.match(command_pattern, line):
			directory = pathlib.Path(filename).parent

			requested_depth = match.group(1)
			command = match.group(2).strip()

			if requested_depth != None and requested_depth != "":
				requested_depth = int(requested_depth)
				max_depth = max(requested_depth, max_depth)
			else:
				requested_depth = 0
			
			if requested_depth != 0 and filename not in files_with_custom_depth:
				files_with_custom_depth.add(filename)
			
			if requested_depth != depth:
				new_contents.append(line)
				continue

			if command.split(" ")[0].strip() in keywords:
				continue

			if ".py" in command:
				command = f"cd {directory} && {get_env_commands()} python3 {command}"
			else:
				command = f"cd {directory} && {get_env_commands()} {command}"

			process = os.popen(command)
			output = process.read()
			if process.close() != None:
				print(f"Encountered preprocessor command error in '{filename}':", file=sys.stderr)
				print(output, file=sys.stderr)
				exit(1)
			else:
				new_contents.append(output)
		else:
			new_contents.append(line)
	return new_contents

def get_env_commands():
	output = ""
	global environment_variables
	for variable, value in environment_variables.items():
		output = f"{output} {variable}='{value}'"
	return output.strip()

def handle_file(file, depth = 0):
	global in_place_extensions
	global extensions
	global glsl_extensions
	
	file = file.replace("\\", "/")
	original_file = file
	file_object = pathlib.Path(file)
	tmp_file = file.replace("./src/", "./tmp/")

	if ".src" in file:
		tmp_file = file.replace(".src", ".gen")

	tmp_file_object = pathlib.Path(tmp_file)

	if depth > 0:
		file = tmp_file # modify files in-place after depth 0

	extension = extension_regex.search(original_file)
	if extension == None:
		return
	else:
		extension = extension.group(0)

	parent = file_object.parent
	tmp_parent = tmp_file_object.parent

	if extension in extensions:
		opened_file = open(file, "r", encoding='utf-8')
		file_contents = opened_file.readlines()
		opened_file.close()

		if extension in glsl_extensions:
			file_contents.insert(0, "R\"\"(\n")
			file_contents.append(")\"\"\n")
		
		if os.path.exists(tmp_file):
			src_time = file_object.stat().st_mtime
			tmp_time = tmp_file_object.stat().st_mtime

			if src_time > tmp_time or depth > 0 or ".src" in file: # recopy file if source file is newer than tmp file
				write_file(tmp_file, preprocess(original_file, file_contents, depth))
		else:
			os.makedirs(tmp_parent, exist_ok=True)
			write_file(tmp_file, preprocess(original_file, file_contents, depth))
		
		# handle headers
		if extension == ".h" and depth == 0:
			handle_headers(original_file, file_contents)
	
	if ".src" in original_file: # copy the generated file over to the /tmp directory
		new_tmp_file = file.replace("./src/", "./tmp/")
		copyfile(tmp_file, new_tmp_file)

if __name__ == "__main__":
	total_files = []
	for root, subdirs, files in os.walk("./src"):
		files = [f"{root}/{file}" for file in files]
		for file in files:
			total_files.append(file)
			handle_file(file)
	
	environment_variables["game_object_type_enums"] = json.dumps(game_object_type_enums)

	for depth in range(1, max_depth + 1):
		for file in files_with_custom_depth:
			handle_file(file, depth)
