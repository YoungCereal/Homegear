#!/bin/bash

set -x

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools
# made possible by:
#   Klaus M Pfeiffer (http://blog.kmp.or.at/2012/05/build-your-own-raspberry-pi-image/)
#   Alex Bradbury (http://asbradbury.org/projects/spindle/)

script_dir="$( cd "$(dirname $0)" && pwd )"

deb_mirror="http://mirrordirector.raspbian.org/raspbian/"
deb_local_mirror=$deb_mirror
deb_release="buster"

bootsize="64M"
buildenv="/root/rpi"
rootfs="${buildenv}/rootfs"
bootfs="${rootfs}/boot"

mydate=`date +%Y%m%d`

if [ $EUID -ne 0 ]; then
  echo "ERROR: This tool must be run as Root"
  exit 1
fi

echo "Creating image..."
mkdir -p $buildenv
image="${buildenv}/rpi_homegear_${deb_release}_${mydate}.img"
dd if=/dev/zero of=$image bs=1MB count=1536
[ $? -ne 0 ] && exit 1
device=`losetup -f --show $image`
[ $? -ne 0 ] && exit 1
echo "Image $image Created and mounted as $device"

fdisk $device << EOF
n
p
1

+$bootsize
t
c
n
p
2


w
EOF
sleep 1

losetup -d $device
[ $? -ne 0 ] && exit 1
sleep 1

device=`kpartx -va $image | sed -E 's/.*(loop[0-9]{1,2})p.*/\1/g' | head -1`
[ $? -ne 0 ] && exit 1
sleep 1

echo "--- kpartx device ${device}"
device="/dev/mapper/${device}"
bootp=${device}p1
rootp=${device}p2

mkfs.vfat $bootp
mkfs.ext4 $rootp

mkdir -p $rootfs
[ $? -ne 0 ] && exit 1

mount $rootp $rootfs
[ $? -ne 0 ] && exit 1

cd $rootfs
debootstrap --no-check-gpg --foreign --arch=armhf $deb_release $rootfs $deb_local_mirror
[ $? -ne 0 ] && exit 1

cp /usr/bin/qemu-arm-static usr/bin/
[ $? -ne 0 ] && exit 1
LANG=C chroot $rootfs /debootstrap/debootstrap --second-stage
[ $? -ne 0 ] && exit 1

mount $bootp $bootfs
[ $? -ne 0 ] && exit 1

# {{{ Only for stretch - correct errors
# Not sure if this is needed for buster
chroot $rootfs apt-get clean
rm -rf $rootfs/var/lib/apt/lists/*
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get update
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y --allow-unauthenticated install debian-keyring debian-archive-keyring
wget http://archive.raspbian.org/raspbian.public.key && chroot $rootfs apt-key add raspbian.public.key && rm raspbian.public.key
wget http://archive.raspberrypi.org/debian/raspberrypi.gpg.key && chroot $rootfs apt-key add raspberrypi.gpg.key && rm raspberrypi.gpg.key
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get update
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install python3
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
# }}}

echo "deb $deb_local_mirror $deb_release main contrib non-free rpi
" > etc/apt/sources.list

echo "blacklist i2c-bcm2708" > $rootfs/etc/modprobe.d/raspi-blacklist.conf

echo "dwc_otg.lpm_enable=0 console=ttyS0,115200 console=tty1 kgdboc=ttyUSB0,115200 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes net.ifnames=0 biosdevname=0 noswap rootwait" > boot/cmdline.txt

rm -f $rootfs/etc/fstab
cat > "$rootfs/etc/fstab" <<'EOF'
proc            /proc                       proc            defaults                                                            0       0
/dev/mmcblk0p1  /boot                       vfat            defaults,noatime,ro                                                 0       2
/dev/mmcblk0p2  /                           ext4            defaults,noatime,ro                                                 0       1
tmpfs           /run                        tmpfs           defaults,nosuid,mode=1777,size=50M                                  0       0
tmpfs           /dev/shm                    tmpfs           defaults,size=32m                                                   0       0
tmpfs           /sys/fs/cgroup              tmpfs           defaults,size=32m                                                   0       0
#tmpfs           /var/<your directory>       tmpfs           defaults,nosuid,mode=1777,size=50M                                  0       0
EOF

#Setup network settings
echo "homegearpi" > etc/hostname
echo "127.0.0.1       localhost
127.0.1.1       homegearpi
::1             localhost ip6-localhost ip6-loopback
ff02::1         ip6-allnodes
ff02::2         ip6-allrouters
" > etc/hosts

echo "# interfaces(5) file used by ifup(8) and ifdown(8)

# Please note that this file is written to be used with dhcpcd
# For static IP, consult /etc/dhcpcd.conf and 'man dhcpcd.conf'

# Include files from /etc/network/interfaces.d:
source-directory /etc/network/interfaces.d
" > etc/network/interfaces
#End network settings

echo "console-common    console-data/keymap/policy      select  Select keymap from full list
console-common  console-data/keymap/full        select  us
" > debconf.set

cat > "$rootfs/third-stage" <<'EOF'
#!/bin/bash
set -x
debconf-set-selections /debconf.set
rm -f /debconf.set
apt-get update
ls -l /etc/apt/sources.list.d
cat /etc/apt/sources.list
apt-get -y install apt-transport-https ca-certificates wget
update-ca-certificates --fresh
mkdir -p /etc/apt/sources.list.d/
echo "deb http://archive.raspberrypi.org/debian/ buster main ui" > /etc/apt/sources.list.d/raspi.list
wget http://archive.raspberrypi.org/debian/raspberrypi.gpg.key
apt-key add - < raspberrypi.gpg.key
rm raspberrypi.gpg.key
apt-get update
apt-get -y install libraspberrypi0 libraspberrypi-bin locales btrfs-tools console-common dhcpcd5 ntpdate fake-hwclock resolvconf openssh-server git-core binutils curl libcurl3-gnutls sudo parted unzip p7zip-full libxml2-utils keyboard-configuration python-lzo libgcrypt20 libgpg-error0 libgnutlsxx28 lua5.2 libenchant1c2a libltdl7 libxslt1.1 libmodbus5 tmux dialog whiptail
# Wireless packets
apt-get -y install bluez-firmware firmware-atheros firmware-libertas firmware-realtek firmware-ralink firmware-brcm80211 wireless-tools wpasupplicant
# Install bootloader and kernel
apt-get -y install raspberrypi-bootloader raspberrypi-kernel
wget http://goo.gl/1BOfJ -O /usr/bin/rpi-update
chmod +x /usr/bin/rpi-update
mkdir -p /lib/modules/$(uname -r)
# Would install test kernel
# rpi-update
rm -Rf /boot.bak
useradd --create-home --shell /bin/bash --user-group pi
echo "pi:raspberry" | chpasswd
echo "pi ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
echo "FILE=/data/fake-hwclock.data" >> /etc/default/fake-hwclock
dpkg-reconfigure locales
service ssh stop

systemctl disable serial-getty@ttyAMA0.service
systemctl disable serial-getty@serial0.service
systemctl disable serial-getty@ttyS0.service

systemctl enable systemd-timesyncd
EOF
chmod +x $rootfs/third-stage
LANG=C chroot $rootfs /third-stage
rm -f $rootfs/third-stage

mkdir -p $rootfs/lib/systemd/scripts
cat > "$rootfs/lib/systemd/scripts/setup-tmpfs.sh" <<'EOF'
#!/bin/bash

if [ ! -z /data/fake-hwclock.data ]; then
    date -u -s "$(cat /data/fake-hwclock.data)"
fi

modprobe zram num_devices=3

echo 134217728 > /sys/block/zram0/disksize
echo 134217728 > /sys/block/zram1/disksize

mkfs.ext4 /dev/zram0
mkfs.ext4 /dev/zram1

mount /dev/zram0 /var/log
mount /dev/zram1 /var/tmp

chmod 775 /var/log
chmod 777 /var/tmp

mkdir /var/tmp/lock
chmod 777 /var/tmp/lock
mkdir /var/tmp/dhcp
chmod 755 /var/tmp/dhcp
mkdir /var/tmp/spool
chmod 755 /var/tmp/spool
mkdir /var/tmp/systemd
chmod 755 /var/tmp/systemd
touch /var/tmp/systemd/random-seed
chmod 600 /var/tmp/systemd/random-seed
mkdir -p /var/spool/cron/crontabs
chmod 731 /var/spool/cron/crontabs
chmod +t /var/spool/cron/crontabs

exit 0
EOF
    chmod 750 $rootfs/lib/systemd/scripts/setup-tmpfs.sh

    cat > "$rootfs/lib/systemd/system/setup-tmpfs.service" <<'EOF'
[Unit]
Description=setup-tmpfs
DefaultDependencies=no
After=-.mount run.mount
Before=systemd-random-seed.service systemd-timesyncd.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/lib/systemd/scripts/setup-tmpfs.sh
TimeoutSec=30s

[Install]
WantedBy=sysinit.target
EOF

cat > "$rootfs/etc/logrotate.conf" <<'EOF'
daily
rotate 0
size 1M
create
compress
include /etc/logrotate.d
/var/log/wtmp {
    missingok
    monthly
    create 0664 root utmp
    rotate 0
}
/var/log/btmp {
    missingok
    monthly
    create 0660 root utmp
    rotate 0
}
EOF

cat > "$rootfs/etc/cron.daily/logrotate" <<'EOF'
#!/bin/sh

test -x /usr/sbin/logrotate || exit 0
/usr/sbin/logrotate -s /data/logrotate-status /etc/logrotate.conf
EOF

cat > "$rootfs/etc/logrotate.d/rsyslog" <<'EOF'
/var/log/syslog
{
	rotate 0
	daily
	size 1M
	missingok
	notifempty
	compress
	postrotate
		invoke-rc.d rsyslog rotate > /dev/null
	endscript
}

/var/log/mail.info
/var/log/mail.warn
/var/log/mail.err
/var/log/mail.log
/var/log/daemon.log
/var/log/kern.log
/var/log/auth.log
/var/log/user.log
/var/log/lpr.log
/var/log/cron.log
/var/log/debug
/var/log/messages
{
	rotate 0
	daily
	size 1M
	missingok
	notifempty
	compress
	sharedscripts
	postrotate
		invoke-rc.d rsyslog rotate > /dev/null
	endscript
}
EOF

cat > "$rootfs/etc/cron.d/restart-ntp" <<'EOF'
SHELL=/bin/bash
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
# Some services like the homegear-dc-connector set the system time.
# After that ntp is not updating the time anymore. To make it work
# again, it needs to be restartet. We are doing this from a cron
# job as a workaround.

7 * * * * root systemctl restart ntp
EOF

cat > "$rootfs/etc/cron.d/btrfs-scrub" <<'EOF'
SHELL=/bin/bash
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
# Run BTRFS scrub on data partition once per month to verify
# checksums.

12 3 1 * * root btrfs scrub start /data
EOF

cat > "$rootfs/fourth-stage" <<'EOF'
rm -Rf /tmp
ln -s /var/tmp /tmp
mkdir /data
rm -Rf /var/spool
ln -s /var/tmp/spool /var/spool
rm -Rf /var/lib/dhcp
ln -s /var/tmp/dhcp /var/lib/dhcp
rm -Rf /var/lock
ln -s /var/tmp/lock /var/lock
rm -Rf /var/lib/systemd
ln -s /var/tmp/systemd /var/lib/systemd

# {{{ debian-fixup fixes
    ln -sf /proc/mounts /etc/mtab
# }}}

systemctl enable setup-tmpfs
sed -i "s/#SystemMaxUse=/SystemMaxUse=1M/g" /etc/systemd/journald.conf
sed -i "s/#RuntimeMaxUse=/RuntimeMaxUse=1M/g" /etc/systemd/journald.conf
EOF
chmod +x $rootfs/fourth-stage
LANG=C chroot $rootfs /fourth-stage
rm $rootfs/fourth-stage

#Install raspi-config
wget https://raw.githubusercontent.com/RPi-Distro/raspi-config/master/raspi-config
mv raspi-config usr/bin
chown root:root usr/bin/raspi-config
chmod 755 usr/bin/raspi-config
#End install raspi-config

#Create Raspberry Pi boot config
echo "# For more options and information see
# http://www.raspberrypi.org/documentation/configuration/config-txt.md
# Some settings may impact device functionality. See link above for details

# uncomment if you get no picture on HDMI for a default "safe" mode
#hdmi_safe=1

# uncomment this if your display has a black border of unused pixels visible
# and your display can output without overscan
#disable_overscan=1

# uncomment the following to adjust overscan. Use positive numbers if console
# goes off screen, and negative if there is too much border
#overscan_left=16
#overscan_right=16
#overscan_top=16
#overscan_bottom=16

# uncomment to force a console size. By default it will be display's size minus
# overscan.
#framebuffer_width=1280
#framebuffer_height=720

# uncomment if hdmi display is not detected and composite is being output
#hdmi_force_hotplug=1

# uncomment to force a specific HDMI mode (this will force VGA)
#hdmi_group=1
#hdmi_mode=1

# uncomment to force a HDMI mode rather than DVI. This can make audio work in
# DMT (computer monitor) modes
#hdmi_drive=2

# uncomment to increase signal to HDMI, if you have interference, blanking, or
# no display
#config_hdmi_boost=4

# uncomment for composite PAL
#sdtv_mode=2

#uncomment to overclock the arm. 700 MHz is the default.
#arm_freq=800

# Uncomment some or all of these to enable the optional hardware interfaces
#dtparam=i2c_arm=on
#dtparam=i2s=on
#dtparam=spi=on

# Uncomment this to enable the lirc-rpi module
#dtoverlay=lirc-rpi

# Additional overlays and parameters are documented /boot/overlays/README

# Enable audio (loads snd_bcm2835)
dtparam=audio=on

enable_uart=1
dtparam=spi=on
dtparam=i2c_arm=on
gpu_mem=16" > boot/config.txt
chown root:root boot/config.txt
chmod 755 boot/config.txt
#End Raspberry Pi boot config

echo "deb $deb_mirror $deb_release main contrib non-free rpi
" > etc/apt/sources.list

cat > "$rootfs/setupPartitions.sh" <<-'EOF'
#!/bin/bash

stage_one()
{
	export LC_ALL=C
	export LANG=C
    bytes=$(fdisk -l | grep mmcblk0 | head -1 | cut -d "," -f 2 | cut -d " " -f 2)
    gigabytes=$(($bytes / 1073741824))
    maxrootpartitionsize=$(($gigabytes - 1))

    rootpartitionsize=""
    while [ -z $rootpartitionsize ] || ! [[ $rootpartitionsize =~ ^[1-9][0-9]*$ ]] || [[ $rootpartitionsize -gt $maxrootpartitionsize ]]; do
        rootpartitionsize=$(dialog --stdout --title "Partitioning" --no-tags --no-cancel --inputbox "Enter new size of readonly root partition in gigabytes. The minimum partition size is 2 GB. The maximum partition size is $maxrootpartitionsize GB. We recommend only using as much space as really necessary so damaged sectors can be placed outside of the used space. It is also recommended to use a SD card as large as possible." 15 50 "2")
        if ! [[ $rootpartitionsize =~ ^[1-9][0-9]*$ ]] || [[ $rootpartitionsize -lt 2 ]]; then
            dialog --title "Partitioning" --msgbox "Please enter a valid size in gigabytes (without unit). E. g. \"2\" or \"4\". Not \"2G\"." 10 50
        fi
    done

    maxdatapartitionsize=$(($gigabytes - $rootpartitionsize))
    datapartitionsize=""
    while [ -z $datapartitionsize ] || ! [[ $datapartitionsize =~ ^[1-9][0-9]*$ ]] || [[ $datapartitionsize -gt $maxdatapartitionsize ]]; do
        datapartitionsize=$(dialog --stdout --title "Partitioning" --no-tags --no-cancel --inputbox "Enter size of writeable data partition in gigabytes. The maximum partition size is $maxdatapartitionsize GB. We recommend only using as much space as really necessary so damaged sectors can be placed outside of the used space. It is also recommended to use a SD card as large as possible." 15 50 "2")
        if ! [[ $datapartitionsize =~ ^[1-9][0-9]*$ ]]; then
            dialog --title "Partitioning" --msgbox "Please enter a valid size in gigabytes (without unit). E. g. \"2\" or \"4\". Not \"2G\"." 10 50
        fi
    done

    fdisk /dev/mmcblk0 << EOC
d
2
n
p
2

+${rootpartitionsize}G
n
p
3

+${datapartitionsize}G
w
EOC

    rm -f /partstageone
    touch /partstagetwo
    sync
    dialog --no-cancel --stdout --title "Partition setup" --no-tags --pause "Rebooting in 10 seconds..." 10 50 10
    reboot
}

stage_two()
{
    TTY_X=$(($(stty size | awk '{print $2}')-6))
    TTY_Y=$(($(stty size | awk '{print $1}')-6))
    resize2fs /dev/mmcblk0p2 | dialog --title "Partition setup" --progressbox "Resizing root partition..." $TTY_Y $TTY_X
    mkfs.btrfs -f -d dup -m dup /dev/mmcblk0p3 | dialog --title "Partition setup" --progressbox "Creating data partition..." $TTY_Y $TTY_X

    sed -i '/\/dev\/mmcblk0p2/a\
\/dev\/mmcblk0p3  \/data                       btrfs            defaults,degraded,compress=lzo,noatime,nodiratime,autodefrag,space_cache,commit=600             0       1' /etc/fstab
    mount -o defaults,degraded,compress=lzo,noatime,nodiratime,autodefrag,space_cache,commit=600 /dev/mmcblk0p3 /data
    sed -i '/^After=/ s/$/ data.mount/' /lib/systemd/system/setup-tmpfs.service
    systemctl daemon-reload
    rm -f /partstagetwo
    sleep 5
}

[ -f /partstagetwo ] && stage_two
[ -f /partstageone ] && stage_one
EOF
touch $rootfs/partstageone
chmod 755 $rootfs/setupPartitions.sh

#Bash profile
echo "let upSeconds=\"\$(/usr/bin/cut -d. -f1 /proc/uptime)\"
let secs=\$((\${upSeconds}%60))
let mins=\$((\${upSeconds}/60%60))
let hours=\$((\${upSeconds}/3600%24))
let days=\$((\${upSeconds}/86400))
UPTIME=\`printf \"%d days, %02dh %02dm %02ds\" \"\$days\" \"\$hours\" \"\$mins\" \"\$secs\"\`

if test -e /usr/bin/homegear; then
    echo \"\$(tput setaf 4)\$(tput bold)
                   dd
                  dddd
                dddddddd
              dddddddddddd
               dddddddddd                   \$(tput setaf 7)Welcome to your Homegear system!\$(tput setaf 4)
               dddddddddd                   \$(tput setaf 7)\`uname -srmo\`\$(tput setaf 4)
               d\$(tput setaf 6).:dddd:.\$(tput setaf 4)d\$(tput setaf 6)
    .:ool:,,:oddddddddddddo:,,:loo:.        \$(tput sgr0)Uptime.............: \${UPTIME}\$(tput setaf 6)\$(tput bold)
    oddddddddddddddddddddddddddddddo        \$(tput sgr0)Homegear Version...: \$(homegear -v | head -1 | cut -d \" \" -f 3)\$(tput setaf 6)\$(tput bold)
    .odddddddd| \$(tput setaf 7)Homegear\$(tput setaf 6) |ddddddddo.
     lddddddddddddddddddddddddddddl
    lddddddddddddddddddddddddddddddl
  .:dddddddddddddc.''.cddddddddddddd:.
:odddddddddddddo.      .odddddddddddddo:
ddddddddddddddd,        ,ddddddddddddddd


\$(tput sgr0)\"
fi

echo \"\"
echo \"* To change data on the root partition (e. g. to update the system),\"
echo \"  enter:\"
echo \"\"
echo \"  rw\"
echo \"\"
echo \"  When you are done, execute\"
echo \"\"
echo \"  ro\"
echo \"\"
echo \"  to make the root partition readonly again.\"
echo \"* You can store data on \\\"/data\\\". It is recommended to only backup\"
echo \"  data to this directory. During operation data should be written to a\"
echo \"  temporary file system. By default these are \\\"/var/log\\\" and \"
echo \"  \\\"/var/tmp\\\". You can add additional mounts in \\\"/etc/fstab\\\".\"
echo \"* Remember to backup all data to \\\"/data\\\" before rebooting.\"
echo \"\"

# if running bash
if [ -n \"\$BASH_VERSION\" ]; then
    # include .bashrc if it exists
    if [ -f \"\$HOME/.bashrc\" ]; then
        . \"\$HOME/.bashrc\"
    fi
fi

# set PATH so it includes user's private bin if it exists
if [ -d \"\$HOME/bin\" ] ; then
    PATH=\"\$HOME/bin:\$PATH\"
fi" > home/pi/.bash_profile
cat home/pi/.bash_profile > root/.bash_profile
#End bash profile

cat >> "$rootfs/etc/bash.bashrc" <<-'EOF'
# set variable identifying the filesystem you work in (used in the prompt below)
set_bash_prompt(){
    fs_mode=$(mount | sed -n -e "s/^\/dev\/.* on \/ .*(\(r[w|o]\).*/\1/p")
    PS1='\[\033[01;32m\]\u@\h${fs_mode:+($fs_mode)}\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '
}
 
# setup fancy prompt"
PROMPT_COMMAND=set_bash_prompt
EOF

cat >> "$rootfs/usr/bin/ro" <<-'EOF'
#/bin/bash
mount -o remount,ro /; mount -o remount,ro /boot
EOF
cat >> "$rootfs/usr/bin/rw" <<-'EOF'
#!/bin/bash
mount -o remount,rw /; mount -o remount,rw /boot
EOF
chmod 755 $rootfs/usr/bin/ro
chmod 755 $rootfs/usr/bin/rw

"${script_dir}/CreateRpiImageBusterCustom.sh" $rootfs

echo "#!/bin/bash
apt-get clean
rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
rm -f cleanup
" > cleanup
chmod +x cleanup
LANG=C chroot $rootfs /cleanup

cd

umount $bootp
umount $rootp

kpartx -d $image

mv $image .
rm -Rf $buildenv

echo "Created Image: $image"

echo "Done."
