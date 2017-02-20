#!/usr/bin/env python

import glob
import os
import platform
import sys
import swiftclient

from ctypes import *
from swiftclient.service import SwiftService, SwiftUploadObject
from sys import argv

_opts = {
    "auth": 'https://cloud.swiftstack.com/auth/v1.0',
    "auth_version": '1.0',
    "user": "dedup",
    "key": "dedup",
}


def clean_temp(temp_slo_folder):
    files = glob.glob(temp_slo_folder + "/*")
    for file in files:
        os.remove(file)


def find_path_delimeter():
    os_system = platform.system()
    if os_system == 'Windows':
        return "\\"
    else:
        return "/"	


def find_dedup_library(name='libclsPyVariableChunk', version=None):
    os_system = platform.system()
    if os_system == 'Windows':
        lib_type = '.dll'
        if version:
            suffix = '-' + version + lib_type
    elif os_system == 'Darwin':
        lib_type = '.dylib'
        if version:
            suffix = '.' + version + lib_type
    else:
        # Use default Linux library
        lib_type = '.so'
        if version:
            suffix = lib_type + '.' + version

    if not version:
        suffix = lib_type

    return (name + suffix)


def create_manifest_chunks(local_file, chunk_folder,
                           library='libclsPyVariableChunk', version=None):
    libname = find_dedup_library(library)
    lib = cdll.LoadLibrary(libname)
    lib.ProcessFileToVar.restype = c_char_p
    return lib.ProcessFileToVar(local_file, 0, 2, 64, 0, True, True,
                                chunk_folder, True)


def upload_manifest(manifest_obj, args):
    try:
        swift_conn = swiftclient.client.Connection(authurl=args["authurl"],
                                                   user=args["user"],
                                                   key=args["key"])
        swift_conn.put_object(
            args["manifest_container"],
            args["filename"], manifest_obj,
            query_string="multipart-manifest=put")
        print("upload ok!")
    except Exception, e:
        print(e)


def upload_dedup_chunks(container, obj_path):
    headers = []
    options = {
        'skip_identical': True,
        'header': headers
    }

    with SwiftService(options=_opts) as swift:
        objects = []
        for chunks in os.listdir(obj_path):
            objects.append(swiftclient.service.SwiftUploadObject(
                obj_path + chunks,
                chunks))
            resp = swift.upload(container, objects=objects, options=options)
            for r in resp:
                print(r)


if __name__ == '__main__':

    if len(sys.argv) == 3:
        # input file for dedup
        source = argv[1]
        temp = argv[2]
        args = {
            "filename": os.path.basename(source),
            "source_file": os.path.abspath(source),
            "chunks_container": "chunks",
            "manifest_container": "dedup",
            "temp_directory": os.path.abspath(temp),
            "authurl": 'https://cloud.swiftstack.com/auth/v1.0',
            "user": "dedup",
            "key": "dedup",
        }

        if not os.path.exists(args["temp_directory"]):
            os.makedirs(args["temp_directory"])
            clean_temp(args["temp_directory"])

        result = create_manifest_chunks(args["source_file"],
                                        args["temp_directory"] + find_path_delimeter())
        upload_dedup_chunks(args["chunks_container"],
                            args["temp_directory"] + find_path_delimeter())
        upload_manifest(result, args)

    else:
        print("please try %s [dedup_file] [temp_slo_folder]" % argv[0])
