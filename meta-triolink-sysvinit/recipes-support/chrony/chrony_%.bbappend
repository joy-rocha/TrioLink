# Troca o chrony.conf padrao do chrony (meta-networking) pelo do TrioLink:
# servidores NTP brasileiros. O SRC_URI da recipe original ja tem
# file://chrony.conf; com este caminho na frente, o nosso arquivo e o usado.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
