#!/bin/python3
# -*- coding: utf-8 -*-

import argparse
import os, re
import subprocess
from datetime import datetime

ROOT_PATH = os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/..")
EXTERNAL_DEPEND_PATH = ROOT_PATH + "/.depend"
PACK_DATE = datetime.now().strftime("%Y%m%d")
DEPEND_FILE = ROOT_PATH + "/doc/requirements.md"
PLATFORM_NO_SDK = [
    "3516ev200",
    "3519",
    "3536a",
    "kunpeng920",
    "linux64",
    "s2lm",
    "sag",
    "sdm632",
    "ssc32xde",
    "ssc335",
    "win32",
    "win64",
]


def ParseArguments():
  """
  Parse command line arguments for the build script.

  Returns:
    argparse.Namespace: Parsed command line arguments.
  """
  parser = argparse.ArgumentParser(description="Build script")
  parser.add_argument("-p", "--platform", type=str, help="Platform to build for", required=True)
  parser.add_argument("-s", "--sdk", type=str, help="SDK to build for", required=False)
  parser.add_argument("-b", "--build-type", type=str, help="Build type", default="Debug")
  parser.add_argument("-t", "--with-unittests", action="store_true", help="Build with unit tests")
  parser.add_argument("-a", "--with-asan", action="store_true", help="Build with address sanitizer")
  parser.add_argument("--with-spdlog", action="store_true", help="Build with spdlog")
  parser.add_argument("--with-av-aux", action="store_true", help="Build with AV AUX")
  parser.add_argument("--with-ffmpeg", action="store_true", help="Build with FFmpeg")
  parser.add_argument("-d", "--custom-def", type=str, help="Custom definitions", required=False)
  parser.add_argument("-l", "--custom-link", type=str, help="Custom link", required=False)
  return parser.parse_args()


def DownloadExternalDependencies(plat=None, sdk=None):
  """
  Download external dependencies from SVN based on the provided platform and SDK.

  Args:
    plat (str, optional): The platform to download dependencies for. Defaults to None.
    sdk (str, optional): The SDK to download dependencies for. Defaults to None.

  Returns:
    None
  """
  with open(DEPEND_FILE, "r") as f:
    pattern = r"\|\s*(.*?)\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|"
    for line in f.readlines():
      match = re.match(pattern, line)
      if match and match.group(1) != "targetfolder":
        targetfolder, svn_url, ver = match.groups()
        # Skip the line if all groups only contain dashes and spaces
        if all(
          re.match(r"^[-\s]*$", group)
          for group in (targetfolder, svn_url, ver)
        ):
          continue

        if targetfolder[:4] == "lib/":
          if sdk is None:
            svn_url = f"{svn_url}/{plat}"
            targetfolder = os.path.join(
              EXTERNAL_DEPEND_PATH, targetfolder, plat
            )
          else:
            svn_url = f"{svn_url}/{plat}/{sdk}"
            targetfolder = os.path.join(
              EXTERNAL_DEPEND_PATH, targetfolder, plat, sdk
            )
        else:
          targetfolder = os.path.join(EXTERNAL_DEPEND_PATH, targetfolder)
        print(f"targetfolder: {targetfolder}, svn_url: {svn_url}, ver: {ver}")

        SvnCheckOut(svn_url, ver, targetfolder)


def SvnCheckOut(url, ver, targetfolder):
  """
  Check out or update a Subversion repository to the specified target folder.

  Args:
    url (str): The URL of the Subversion repository.
    ver (str): The revision number or tag to check out or update to.
    targetfolder (str): The path to the target folder where the repository will be checked out or updated.

  Returns:
    subprocess.CompletedProcess: The result of the checkout or update operation.

  Raises:
    subprocess.CalledProcessError: If the checkout or update operation fails.

  """
  if os.path.exists(targetfolder):
    print(f"Folder {targetfolder} already exists, skip checkout")
    # try update
    cmd = f"svn update -r {ver}"
    print(cmd)
    return subprocess.run(cmd, shell=True, check=True, cwd=targetfolder)

  cmd = f"svn checkout -r {ver} {url} ."
  print(cmd)
  retry_cnt = 3
  while (
    subprocess.run(cmd, shell=True, check=True, cwd=targetfolder).returncode != 0
    and retry_cnt > 0
  ):
    print(f"Retry svn checkout {retry_cnt}")
    retry_cnt -= 1
  if retry_cnt == 0:
    print(f"Failed to checkout {url}")
    cmd = "rm -rf ."
    subprocess.run(cmd, shell=True, check=True, cwd=targetfolder)
    exit(1)


if __name__ == "__main__":
    args = ParseArguments()
    if args.platform not in PLATFORM_NO_SDK and args.sdk == None:
        print("Please specify the SDK for the platform")
        exit(1)

    print(args)
    print(ROOT_PATH)
    print(EXTERNAL_DEPEND_PATH)
    print(PACK_DATE)
    exit(0)

    # create depend directory
    if not os.path.exists(EXTERNAL_DEPEND_PATH):
        os.makedirs(EXTERNAL_DEPEND_PATH)
    DownloadExternalDependencies(args.platform, args.sdk)
