#!/system/bin/sh
# Split/Second: Velocity - Revert to Default Android Governors & Power Scaling
# Restores stock dynamic frequency scaling for unbiased baseline benchmarking

echo "[SSV Default] Restoring default stock governors and frequency scaling..."

# 1. Restore CPU governors to schedutil
for cpu in /sys/devices/system/cpu/cpu[0-7]; do
    if [ -f "$cpu/cpufreq/scaling_governor" ]; then
        echo schedutil > "$cpu/cpufreq/scaling_governor"
    fi
done

# 2. Restore GPU to msm-adreno-tz with dynamic scaling (160 MHz - 647 MHz)
if [ -d "/sys/class/kgsl/kgsl-3d0" ]; then
    echo msm-adreno-tz > /sys/class/kgsl/kgsl-3d0/devfreq/governor
    echo 160000000 > /sys/class/kgsl/kgsl-3d0/devfreq/min_freq 2>/dev/null
    echo 647000000 > /sys/class/kgsl/kgsl-3d0/devfreq/max_freq 2>/dev/null
    echo 0 > /sys/class/kgsl/kgsl-3d0/force_bus_on 2>/dev/null
    echo 0 > /sys/class/kgsl/kgsl-3d0/force_rail_on 2>/dev/null
    echo 0 > /sys/class/kgsl/kgsl-3d0/force_clk_on 2>/dev/null
fi

# 3. Restore Virtual Memory default
echo 60 > /proc/sys/vm/swappiness 2>/dev/null

# 4. Status Report
echo "[SSV Default] Active CPU Governor:"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "[SSV Default] Active GPU Governor:"
cat /sys/class/kgsl/kgsl-3d0/devfreq/governor
echo "[SSV Default] Active GPU Clock (Hz):"
cat /sys/class/kgsl/kgsl-3d0/devfreq/cur_freq
echo "[SSV Default] Stock Android power governors active."
