#!/system/bin/sh
# Split/Second: Velocity - Snapdragon 660 / Adreno 512 Hardware Turbo Script
# Locks CPU and GPU frequencies to maximum for hitch-free 60 FPS physics and rendering

echo "[SSV Turbo] Initializing hardware performance mode..."

# 1. Lock CPU frequencies to max performance
for cpu in /sys/devices/system/cpu/cpu[0-7]; do
    if [ -f "$cpu/cpufreq/scaling_governor" ]; then
        echo performance > "$cpu/cpufreq/scaling_governor"
    fi
done

# 2. Lock GPU to maximum clock (647 MHz on Adreno 512)
if [ -d "/sys/class/kgsl/kgsl-3d0" ]; then
    echo performance > /sys/class/kgsl/kgsl-3d0/devfreq/governor
    echo 647000000 > /sys/class/kgsl/kgsl-3d0/devfreq/min_freq
    echo 647000000 > /sys/class/kgsl/kgsl-3d0/devfreq/max_freq
    echo 1 > /sys/class/kgsl/kgsl-3d0/force_bus_on 2>/dev/null
    echo 1 > /sys/class/kgsl/kgsl-3d0/force_rail_on 2>/dev/null
    echo 1 > /sys/class/kgsl/kgsl-3d0/force_clk_on 2>/dev/null
fi

# 3. Optimize Virtual Memory
echo 10 > /proc/sys/vm/swappiness 2>/dev/null

# 4. Status Report
echo "[SSV Turbo] Active CPU Governor:"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "[SSV Turbo] Active GPU Governor:"
cat /sys/class/kgsl/kgsl-3d0/devfreq/governor
echo "[SSV Turbo] Active GPU Clock (Hz):"
cat /sys/class/kgsl/kgsl-3d0/devfreq/cur_freq
echo "[SSV Turbo] Snapdragon 660 / Adreno 512 locked at peak performance."
