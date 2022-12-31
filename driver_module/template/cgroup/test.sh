ps -aux | grep top | awk '{print $2}' | sudo xargs -i kill {}
sudo systemd-run --unit=toptest --slice=test top -b
