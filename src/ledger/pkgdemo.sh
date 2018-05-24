#!/usr/bin/env bash
#*******************************************************************************
#*   (c) 2018 ZondaX GmbH
#*
#*  Licensed under the Apache License, Version 2.0 (the "License");
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*      http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an "AS IS" BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#********************************************************************************
SCRIPT_DIR=$(cd $(dirname $0) && pwd)

# Copy hex file
cp ${SCRIPT_DIR}/bin/app.hex ${SCRIPT_DIR}/pkgdemo/app.hex

APPNAME=$1
APPVERSION=$2
ICONNAME=$3

TARGET_ID=0x31100003
DATASIZE=$(cat ${SCRIPT_DIR}/debug/app.map |grep _nvram_data_size | tr -s ' ' | cut -f2 -d' ')
ICONHEX=$(python ${BOLOS_SDK}/icon.py ${ICONNAME} hexbitmaponly)

cat >${SCRIPT_DIR}/pkgdemo/loaddemo.sh <<EOL
#!/usr/bin/env bash
pip install -U setuptools ledgerblue==0.1.17

SCRIPT_DIR=\$(cd \$(dirname \$0) && pwd)
python -m ledgerblue.loadApp --appFlags 0x00 --tlv --targetId ${TARGET_ID} --delete --fileName \${SCRIPT_DIR}/app.hex --appName ${APPNAME} --appVersion ${APPVERSION} --icon ${ICONHEX} --dataSize ${DATASIZE}
EOL

chmod +x ${SCRIPT_DIR}/pkgdemo/loaddemo.sh
