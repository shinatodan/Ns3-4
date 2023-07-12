LINE_NOTIFY_API="https://notify-api.line.me/api/notify"
TOKEN="Ouu8Lp3Aq9gDEdJjMY0XaTxtBSDRo0sfB5s76BEfVey"


rm -rf ~/dataTemp
rm -rf ~/Simulation/* #dataファイルの下を削除する
mkdir ~/"Simulation/GPSR"
mkdir ~/"Simulation/NGPSR"


start_time=`date +%s` 

i=1 #loop
r=1000 #実験回数

for protocol in GPSR NGPSR
do

	mkdir -p ~/"dataTemp"
	while [ $i -le $r ]; do
	
	echo "-run $i  --RoutingProtocol=$protocol "
	./waf --run "shinato-simulator --protocolName=$protocol "
	
	mv ~/dataTemp/data.txt ~/dataTemp/data$i.txt
	mv ~/dataTemp/data$i.txt ~/Simulation/$protocol
	i=`expr $i + 1`
	
	done
	
	i=1
	rm -rf ~/dataTemp	

done

./shinatomaketable $r

end_time=`date +%s` #unix時刻から現在の時刻までの秒数を取得

  SS=`expr ${end_time} - ${start_time}` #シュミレーションにかかった時間を計算する　秒数
  HH=`expr ${SS} / 3600` #時を計算
  SS=`expr ${SS} % 3600`
  MM=`expr ${SS} / 60` #分を計算
  SS=`expr ${SS} % 60` #秒を計算

  echo "シュミレーション時間${HH}:${MM}:${SS}" #シミュレーションにかかった時間を　時:分:秒で表示する

  send_line_notification() {
    message="シミュレーションが終了しました。"
    curl -X POST -H "Authorization: Bearer $TOKEN" -F "message=$message" $LINE_NOTIFY_API
}

# LINE通知を送信
send_line_notification
