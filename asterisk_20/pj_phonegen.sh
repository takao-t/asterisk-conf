#!/bin/sh

SALT=`dd if=/dev/urandom bs=512 count=1|md5sum`
SECBASE="somewhere:$SALT:someone:$1"
ACCCODE=`echo $SECBASE | md5sum | cut -f1,1 -d' '`

#テンプレート(共通部分)
#通常電話機
echo ";電話機用テンプレート(共通設定)"
echo "[phone-defaults](!)"
echo "type=wizard"
echo "transport = transport-udp"
echo "accepts_registrations = yes"
echo "sends_registrations = no"
echo "accepts_auth = yes"
echo "sends_auth = no"
echo "endpoint/context = default"
echo "endpoint/dtmf_mode = rfc4733"
echo "endpoint/call_group = 1"
echo "endpoint/pickup_group = 1"
echo "endpoint/language = ja"
echo "endpoint/disallow = all"
echo "endpoint/allow = ulaw"
echo "endpoint/rtp_symmetric = yes"
echo "endpoint/force_rport = yes"
echo "endpoint/direct_media = no"
echo "endpoint/send_pai = yes"
echo "endpoint/send_rpid = yes"
echo "endpoint/rewrite_contact = yes"
echo "endpoint/inband_progress = yes"
echo "endpoint/allow_subscribe = yes"
echo "aor/max_contacts = 2"
echo "aor/qualify_frequency = 30"
echo "aor/authenticate_qualify = no"
echo "aor/remove_existing = yes"
echo ""
#ブラウザ電話機
echo ";ブラウザ電話機用テンプレート(共通設定)"
echo ";WebSocketタイプ"
echo "[browserphone-defaults](!)"
echo "type=wizard"
echo "transport = transport-wss"
echo "accepts_registrations = yes"
echo "sends_registrations = no"
echo "accepts_auth = yes"
echo "sends_auth = no"
echo "endpoint/context = default"
echo "endpoint/dtmf_mode = rfc4733"
echo "endpoint/call_group = 1"
echo "endpoint/pickup_group = 1"
echo "endpoint/language = ja"
echo "endpoint/disallow = all"
echo "endpoint/allow = ulaw,g722,vp8,vp9,h264"
echo "endpoint/direct_media = no"
echo "endpoint/rtp_timeout = 120"
echo "endpoint/use_avpf = yes"
echo "endpoint/ice_support = yes"
echo "endpoint/media_encryption = dtls"
echo "endpoint/send_pai = yes"
echo "endpoint/send_rpid = yes"
echo "endpoint/rewrite_contact = yes"
echo "endpoint/inband_progress = yes"
echo "endpoint/allow_subscribe = yes"
echo "endpoint/rtcp_mux = yes"
echo "endpoint/dtls_verify = fingerprint"
echo "endpoint/dtls_setup = actpass"
echo "endpoint/dtls_cert_file = /etc/asterisk/keys/asterisk.crt"
echo "endpoint/dtls_private_key = /etc/asterisk/keys/asterisk.key"
echo "endpoint/dtls_ca_file = /etc/asterisk/keys/asterisk.crt"
echo "aor/max_contacts = 1"
echo "aor/qualify_frequency = 60"
echo "aor/authenticate_qualify = no"
echo "aor/remove_existing = yes"
echo ""


echo ";各電話機個別設定"
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
do
    echo "[phone$i](phone-defaults)"
    echo "inbound_auth/username = phone$i"
    SALT=`date +%N`
    SECBASE="phone:$SALT:$i"
    SECRET=`echo $SECBASE | md5sum | cut -f1,1 -d' '`
    echo "inbound_auth/password = $SECRET"
    echo ""
done

echo ";各ブラウザ電話機個別設定"
for i in 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32
do
    echo "[phone$i](browserphone-defaults)"
    echo "inbound_auth/username = phone$i"
    SALT=`date +%N`
    SECBASE="phone:$SALT:$i"
    SECRET=`echo $SECBASE | md5sum | cut -f1,1 -d' '`
    echo "inbound_auth/password = $SECRET"
    echo ""
done
