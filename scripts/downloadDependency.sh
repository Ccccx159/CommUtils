#!/bin/bash

root_dir=$(cd `dirname $0` && pwd)/../
pack_date=$(date "+%Y%m%d")
pack_time=$(date "+%H%M%S")

# 无sdk的平台
platform_without_sdk=("3516ev200" "3519" "3536a" "kunpeng920" "linux64" "s2lm" "sag" "sdm632" "ssc32xde" "ssc335" "win32" "win64")
# download info
download_target_folder=${root_dir}/.tmp
download_require_file=${root_dir}/doc/requirements.md
download_require_array=$(awk '$4 ~ /http.*/ {print $2}' ${download_require_file})


function usage () {
    local space_symbol=" "
    printf "Usage:\n"
    printf "sh ./$(basename $0) [-p platform] [-k sdk] [-s solution] [-d ] [-o ] [-a] [-l] [-w] [-v] [-h] [-c]\n"
    printf "%-10s%-10s%s\n" "${space_symbol}" "-p" "target's platform"
    printf "%-10s%-10s%s\n" "${space_symbol}" "-k" "target's sdk"
    printf "%-10s%-10s%s\n" "${space_symbol}" "-s" "current solution name"
    printf "%-10s%-10s%s\n" "${space_symbol}" "-d" "custom maco definitions. e.g. -d \"-DUSE_LV -DOSA_MODULE_NAME='\\\"denmap\\\"'\""
    printf "%-10s%-10s%s\n" "${space_symbol}" "-o" "custom compile options. e.g. -o \"-fsanitize=address\""
    printf "%-10s%-10s%s\n" "${space_symbol}" "-a" "option for asan_check, default is \"OFF\""
    printf "%-10s%-10s%s\n" "${space_symbol}" "-l" "option for with_dlopen, default is \"OFF\""
    printf "%-10s%-10s%s\n" "${space_symbol}" "-w" "option for download_from_weci, default is \"OFF\""
    printf "%-10s%-10s%s\n" "${space_symbol}" "-v" "option for with_vdec, default is \"OFF\""
    printf "%-10s%-10s%s\n" "${space_symbol}" "-h" "show this script's usage"
    printf "%-10s%-10s%s\n" "${space_symbol}" "-c" "be careful!!! this option will clear all the temporary files"
}

function log () {
    TIME=$(date "+%Y-%m-%d %H:%M:%S")
    
    if [[ "Info" == "$1" ]]; then
        printf "\033[0;32m"
        elif [[ "Debug" == "$1" ]]; then
        printf "\033[0;35m"
        elif [[ "Warn" == "$1" ]]; then
        printf "\033[0;33m"
        elif [[ "Error" == "$1" ]]; then
        printf "\033[5;41;37m"
    fi
    printf "%s [%-5s]   %s\033[0m\n" "${TIME}" "$1" "$2"
}

# 截取文件名
function fn_GetFileName () {
  local file=$1
  local name=${file##*/}
  echo ${name}
  return $?
}

function fn_Clean () {
    log Warn "this operation will clean all the temporary files"
    read -r -p "Are You Sure To Begin Cleanning? [yY/nN](default nN) " input
    case $input in
        [yY][eE][sS]|[yY])
            log Warn "Begin To Cleanning...\n"
            rm ${root_dir}/build -rvf;
            rm ${root_dir}/.tmp -rvf;
            rm ${root_dir}/artifacts -rvf;
            exit 0;
        ;;
        
        [nN][oO]|[nN])
            log Warn "Do Nothing! Will Exit Safely..."
            exit 0;
        ;;
        
        "")
            log Warn "Do Nothing! Will Exit Safely..."
            exit 0;
        ;;

        *)
            log Error "Invalid input..."
            exit -1
        ;;
    esac
}

function fn_ParseArgs () {
    solution=""
    asan_check="OFF"
    with_dlopen="OFF"
    download_from_weci="OFF"
    with_vdec="OFF"
    while getopts "p:k:s:d:o:awvlhc" opts; do
        case $opts in
            p)
                platform=$OPTARG
            ;;
            k)
                sdk=$OPTARG
            ;;
            s)
                solution=$OPTARG
            ;;
            d)
                extra_def=$OPTARG
            ;;
            o)
                extra_opt=$OPTARG
            ;;
            a)
                asan_check="ON"
            ;;
            l)
                with_dlopen="ON"
            ;;
            w)
                download_from_weci="ON"
            ;;
            v)
                with_vdec="ON"
            ;;
            h)
                usage
                exit 0
            ;;
            c)
                fn_Clean
                exit 0
            ;;
            \?)
                log Error "Invalid option: -$OPTARG"
                usage
                exit 1
            ;;
            :)
                log Error "Option [-$OPTARG] requires an argument"
                usage
                exit 1
            ;;
        esac
    done
    if [[ -z $platform ]]; then
        log Error "Args -p <platform> is Required! Please input your target's platform!"
        usage
        exit 1
    fi
    if ! [[ "${platform_without_sdk[@]}" =~ (.*)${platform}(.*) ]] && [[ -z $sdk ]]; then
        log Error "platform[${platform}] needs a sdk name, please input ${platform}'s sdk!"
        usage
        exit 1
    fi
}

function fn_downloadRequirements() {
    for require_opt in ${download_require_array[@]}
    do
        local require_target_folder=${download_target_folder}/${require_opt}
        # 部分依赖项中带有 "/" 在awk中进行正则匹配时需要转义，否则匹配会匹配失败，增加 tmp_require_opt 存放转义后字符串，防止污染源字符串
        local tmp_require_opt
        if [[ "${require_opt}" =~ (.*)/(.*) ]]; then
            tmp_require_opt=${require_opt//\//\\\/}
        else
            tmp_require_opt=${require_opt}
        fi
        local svn_url=$(awk "\$2 ~ /${tmp_require_opt}/ {print \$4}" ${download_require_file})
        local svn_ver=$(awk "\$2 ~ /${tmp_require_opt}/ {print \$6}" ${download_require_file})
        # 依赖项为库时，需在路径上追加平台和sdk，部分平台没有sdk，则仅追加平台名称
        if [[ "${require_opt}" =~ lib(.*) ]]; then
            if ! [[ "${platform_without_sdk[@]}" =~ (.*)${platform}(.*) ]]; then
                require_target_folder="${require_target_folder}/${platform}/${sdk}"
                svn_url="${svn_url}/${platform}/${sdk}"
            else
                require_target_folder="${require_target_folder}/${platform}"
                svn_url="${svn_url}/${platform}"
            fi
        fi
        # 判断当前是否已存在依赖目录，若不存在则尝试check out
        if [[ ! -d ${require_target_folder} ]]; then
            mkdir -p ${require_target_folder}
            local retry_cnt=0
            local co_res=0
            echo "svn co ${svn_url} -r ${svn_ver} ${require_target_folder}"
            svn co ${svn_url} -r ${svn_ver} ${require_target_folder}
            co_res=$?
            # 可能存在网络波动导致拉取失败，则重试 3 次
            while [[ co_res -ne 0 && retry_cnt -lt 3 ]]
            do
                retry_cnt+=1
                echo "svn checkout failed... try again..."
                rm ${require_target_folder} -rf;
                svn co ${svn_url} -r ${svn_ver} ${require_target_folder}
                co_res=$?
            done
            if [[ co_res -ne 0 ]]; then
                echo ${require_target_folder}" svn check out still failed!!!!"
                echo "Roll Back this operation..."
                rm ${require_target_folder} -rf;
                exit -1
            fi
        else
            echo ${require_opt}" already exist, do not check out..."
        fi
    done
}

function fn_printMassage() {
    local space_symbol="------"
    log Info "pack massage:"
    printf "%-7s%-15s%s\n" "${space_symbol}" "solution:" "${solution}"
    printf "%-7s%-15s%s\n" "${space_symbol}" "platform:" "${platform}"
    printf "%-7s%-15s%s\n" "${space_symbol}" "sdk:" "${sdk}"
    printf "%-7s%-15s%s\n" "${space_symbol}" "pack date:" "$(date "+%Y-%m-%d")"
    printf "%-7s%-15s%s\n" "${space_symbol}" "pack time:" "$(date "+%H:%M:%S")"

}

function fn_compile() {
    if [[ -d ${root_dir}/artifacts ]]; then
        rm ${root_dir}/artifacts -rf;
    fi
    if [[ -d ${root_dir}/build ]]; then
        rm ${root_dir}/build -rf;
    fi
    mkdir ${root_dir}/build; cd ${root_dir}/build;
    cmake -DPLATFORM=${platform} -DSDK=${sdk} -DMODULE_NAME=${solution} -DWITH_VDEC=${with_vdec} -DWITH_ASAN_CHECK=${asan_check} -DWITH_DLOPEN=${with_dlopen} -DCUSTOM_DEFINE="${extra_def}" -DCUSTOM_OPTION="${extra_opt}" ../;
    make clean && make VERBOSE=1 -j$(nproc);
    if [[ "$?" != "0" ]]; then
        log Error "make failed! exit now..."
        exit -1;
    fi
    make install;
    cd ${root_dir};    
}

fn_ParseArgs "$@"
fn_downloadRequirements
fn_printMassage

# # 不使用dlopen方式，显式链接算法方案so，从weci下载
# mkdir -p ${root_dir}/.tmp/${solution}/${platform}
# if [[ "${download_from_weci}" == "ON" ]]; then
#     solution_artifacts=$(ls ${root_dir}/.tmp/${solution}/${platform}/*.tar.gz)
#     if [[ -z $solution_artifacts ]]; then
#         # 2023.08.01 Update: WECI no longer support personal invocation of interfaces
#         log Error "Can't find algorithm pacakge in this dir ${root_dir}/.tmp/${solution}/${platform}"
#         # 导入环境
#         # source ./set_env.sh
#         # python3 get_artifacts_from_weci.py "${root_dir}/.tmp/${solution}/${platform}"
#         # download_res=$?
#         # if [[ "$download_res" != "0" ]]; then 
#         #     log Error "download solution's artifacts from weci failed! Please check the log"
#         #     exit -1
#         # fi
#     fi
#     rm ${root_dir}/libs/* -rf;
#     tar -xzvf ${root_dir}/.tmp/${solution}/${platform}/*.tar.gz -C ${root_dir}/.tmp/${solution}/${platform}
#     find ${root_dir}/.tmp/${solution}/${platform}/ -name "lib*.so" -print0 | xargs -0 -l1 -I {} cp {} ${root_dir}/libs
# fi

# fn_compile


# cd ${root_dir}
# root_dir=$(pwd)
# # Copy the cfg files from algorithm package to the artifacts dir.
# # Support directory names 'cfg', 'config' or 'confPath'.
# if [[ $(find ${root_dir}/.tmp/${solution}/${platform} -maxdepth 1 \( -type d -name "cfg" -o -name "config" -o -name "confPath" \) 2> /dev/null | wc -l) -gt 0 ]]; then
#     find ${root_dir}/.tmp/${solution}/${platform} -maxdepth 1 \( -type d -name "cfg" -o -name "config" -o -name "confPath" \) -print0 | xargs -0 -l1 -I {} cp {} ${root_dir}/artifacts/Release/export/ -r
#     config_dir=$(find ${root_dir}/.tmp/${solution}/${platform} -maxdepth 1 \( -type d -name "cfg" -o -name "config" -o -name "confPath" \))
#     config_dir=$(fn_GetFileName ${config_dir})
#     if [[ "$?" == "0" ]]; then
#         echo "-- Installing: ${root_dir}/artifacts/Release/export/${config_dir}"
#         ls ${root_dir}/artifacts/Release/export/${config_dir}/ | xargs -I {} echo "-- Installing: ${root_dir}/artifacts/Release/export/${config_dir}/{}"
#     fi
# else
# # Compatible with the case where the "cfg" directory or the "config" directory does not exist in history versions.
#     find ${root_dir}/.tmp/${solution}/${platform} -maxdepth 1 -type f -not -name "*.tar.gz" -print0 | xargs -0 -l1 -I {} cp {} ${root_dir}/artifacts/Release/export/ -r
#     if [[ "$?" == "0" ]]; then
#         ls ${root_dir}/.tmp/${solution}/${platform} -p | grep -v "/" | grep -v "tar.gz" | xargs -I {} echo "-- Installing: ${root_dir}/artifacts/Release/export/{}"
#     fi
# fi
# if [[ $(find ${root_dir}/.tmp/${solution}/${platform} -name "model" -type d 2> /dev/null | wc -l) -gt 0 ]]; then
#     find ${root_dir}/.tmp/${solution}/${platform} -name "model" -type d -print0 | xargs -0 -l1 -I {} cp {} ${root_dir}/artifacts/Release/export/ -r
#     if [[ "$?" == "0" ]]; then
#         echo "-- Installing: ${root_dir}/artifacts/Release/export/model"
#         ls ${root_dir}/artifacts/Release/export/model/ | xargs -I {} echo "-- Installing: ${root_dir}/artifacts/Release/export/model/{}"
#     fi
# fi

# # cd ${root_dir}/artifacts/Release
# # tar -czvf ${solution}.${platform}.${sdk}.${pack_date}.tar.gz -C ./export/ .