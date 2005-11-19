#!/bin/sh

user=${FM_USER:=`whoami`}
name=${FM_NAME:=`grep "^${user}:" /etc/passwd | cut -d: -f5|sed -e 's/^.*,//'`}
domain=${FM_DOMAIN:=mail.berlios.de}
email="$user@$domain"
packager="$name <$email>"

LANG=C
LC_TIME=C
export LANG LC_TIME

cat <<EOF
# Note: Do not hack fetchmail.spec by hand -- it's generated by specgen.sh
%define with_python 1

Name:		fetchmail
Version:	$1
Release:	1
Vendor:		The Community Fetchmail Project
Packager:	$packager
URL:		http://developer.berlios.de/projects/fetchmail
Source:		%{name}-%{version}.tar.bz2
Group:		Applications/Mail
Group(pt_BR):   Aplica��es/Correio Eletr�nico
License:	GPL
Icon:		fetchmail.xpm
%if "%{_vendor}" == "suse"
Requires:	smtp_daemon
%else
Requires:	smtpdaemon
%endif
BuildPrereq:	gettext-devel openssl-devel
BuildRoot:	/var/tmp/%{name}-%{version}
Summary:	Full-featured POP/IMAP mail retrieval daemon
Summary(fr):	Collecteur (POP/IMAP) de courrier �lectronique
Summary(de):	Program zum Abholen von E-Mail via POP/IMAP
Summary(pt):	Busca mensagens de um servidor usando POP ou IMAP
Summary(es):	Recolector de correo via POP/IMAP
Summary(pl):	Zdalny demon pocztowy do protoko��w POP2, POP3, APOP, IMAP
Summary(tr):	POP2, POP3, APOP, IMAP protokolleri ile uzaktan mektup alma yaz�l�m�
Summary(da):	Alsidig POP/IMAP post-afhentnings d�mon
BuildRoot: %{_tmppath}/%{name}-root
#Keywords: mail, client, POP, POP2, POP3, APOP, RPOP, KPOP, IMAP, ETRN, ODMR, SMTP, ESMTP, GSSAPI, RPA, NTLM, CRAM-MD5, SASL
#Destinations:	fetchmail-users@lists.berlios.de, fetchmail-announce@lists.berlios.de

%description
Fetchmail is a free, full-featured, robust, and well-documented remote
mail retrieval and forwarding utility intended to be used over
on-demand TCP/IP links (such as SLIP or PPP connections).  It
retrieves mail from remote mail servers and forwards it to your local
(client) machine's delivery system, so it can then be be read by
normal mail user agents such as mutt, elm, pine, (x)emacs/gnus, or mailx.
Comes with an interactive GUI configurator suitable for end-users.

%description -l fr
Fetchmail est un programme qui permet d'aller rechercher du courrier
�lectronique sur un serveur de mail distant. Fetchmail connait les
protocoles POP (Post Office Protocol), IMAP (Internet Mail Access
Protocol) et d�livre le courrier �lectronique a travers le
serveur SMTP local (habituellement sendmail).

%description -l de
Fetchmail ist ein freies, vollst�ndiges, robustes und
wohldokumentiertes Werkzeug zum Abholen und Weiterleiten von E-Mail,
zur Verwendung �ber tempor�re TCP/IP-Verbindungen (wie
z.B. SLIP- oder PPP-Verbindungen).  Es holt E-Mail von
entfernten Mail-Servern ab und reicht sie an das Auslieferungssystem
der lokalen Client-Maschine weiter, damit sie dann von normalen MUAs
("mail user agents") wie mutt, elm, pine, (x)emacs/gnus oder mailx
gelesen werden k�nnen.  Ein interaktiver GUI-Konfigurator f�r
Endbenutzer wird mitgeliefert.

%description -l pt
Fetchmail � um programa que � usado para recuperar mensagens de um
servidor de mail remoto. Ele pode usar Post Office Protocol (POP)
ou IMAP (Internet Mail Access Protocol) para isso, e entrega o mail
atrav�s do servidor local SMTP (normalmente sendmail).
Vem com uma interface gr�fica para sua configura��o. 

%description -l es
Fetchmail es una utilidad gratis, completa, robusta y bien documentada
para la recepci�n y reenv�o de correo pensada para ser usada en
conexiones TCP/IP temporales (como SLIP y PPP). Recibe el correo de
servidores remotos y lo reenv�a al sistema de entrega local, siendo de
ese modo posible leerlo con programas como mutt, elm, pine, (x)emacs/gnus
o mailx. Contiene un configurador GUI interactivo pensado para usuarios.

%description -l pl
Fetchmail jest programem do �ci�gania poczty ze zdalnych serwer�w
pocztowych. Do �ci�gania poczty mo�e on uzywa� protoko��w POP (Post Office
Protocol) lub IMAP (Internet Mail Access Protocol). �ci�gni�t� poczt�
dostarcza do ko�cowych odbiorc�w poprzez lokalny serwer SMTP.

%description -l tr
fetchmail yaz�l�m�, POP veya IMAP deste�i veren bir sunucuda yer alan
mektuplar�n�z� al�r.

%description -l da
Fetchmail er et gratis, robust, alsidigt og vel-dokumenteret v�rkt�j 
til afhentning og videresending af elektronisk post via TCP/IP
baserede opkalds-forbindelser (s�som SLIP eller PPP forbindelser).   
Den henter post fra en ekstern post-server, og videresender den
til din lokale klient-maskines post-system, s� den kan l�ses af
almindelige mail klienter s�som mutt, elm, pine, (x)emacs/gnus,
eller mailx. Der medf�lger ogs� et interaktivt GUI-baseret
konfigurations-program, som kan bruges af almindelige brugere.

%if %{with_python}
%package -n fetchmailconf
Summary:	A GUI configurator for generating fetchmail configuration files
Summary(de):	GUI-Konfigurator f�r fetchmail
Summary(pl):	GUI konfigurator do fetchmaila
Summary(fr):	GUI configurateur pour fetchmail
Summary(es):	Configurador GUI interactivo para fetchmail
Summary(pt):	Um configurador gr�fico para o fetchmail
Group:		Utilities/System
Group(pt):	Utilit�rios/Sistema
BuildPrereq:	python
Requires:	%{name} = %{version}, python

%description -n fetchmailconf
A GUI configurator for generating fetchmail configuration file written in
Python.

%description -n fetchmailconf -l de
Ein in Python geschriebenes Programm mit graphischer Oberfl�che zur
Erzeugung von Fetchmail-Konfigurationsdateien.

%description -n fetchmailconf -l pt
Um configurador gr�fico para a gera��o de arquivos de configura��o do
fetchmail. Feito em python.

%description -n fetchmailconf -l es
Configurador gr�fico para fetchmail escrito en python

%description -n fetchmailconf -l de
Ein interaktiver GUI-Konfigurator f�r fetchmail in python

%description -n fetchmailconf -l pl
GUI konfigurator do fetchmaila napisany w pythonie.
%endif

%prep
%setup -q

%build
LDFLAGS="-s"
export CFLAGS LDFLAGS
%configure --without-included-gettext --without-kerberos --with-ssl --enable-inet6
make

%install
rm -rf \$RPM_BUILD_ROOT
make install-strip DESTDIR=\$RPM_BUILD_ROOT

%if %{with_python}
mkdir -p \$RPM_BUILD_ROOT/usr/lib/rhs/control-panel
cp rh-config/*.{xpm,init} \$RPM_BUILD_ROOT/usr/lib/rhs/control-panel
mkdir -p \$RPM_BUILD_ROOT/etc/X11/wmconfig
cp rh-config/fetchmailconf.wmconfig \$RPM_BUILD_ROOT/etc/X11/wmconfig/fetchmailconf
%endif

chmod 644 contrib/*

%clean
rm -rf \$RPM_BUILD_ROOT

%files
%defattr (644, root, root, 755)
%doc ABOUT-NLS FAQ COPYING FEATURES NEWS
%doc NOTES OLDNEWS README README.SSL BUGS
%doc contrib
%doc fetchmail-features.html fetchmail-FAQ.html esrs-design-notes.html
%doc design-notes.html
%attr(644, root, man) %{_mandir}/man1/fetchmail.1*
%attr(755, root, root) %{_bindir}/fetchmail
%attr(644,root,root) %{_datadir}/locale/*/LC_MESSAGES/fetchmail.mo

%if %{with_python}
%files -n fetchmailconf
%defattr (644, root, root, 755)
%attr(644,root,root) /etc/X11/wmconfig/fetchmailconf
%attr(755,root,root) %{_bindir}/fetchmailconf
%attr(644, root, man) %{_mandir}/man1/fetchmailconf.1*
%attr(755,root,root) %{_prefix}/lib/python*/site-packages/fetchmailconf.py*
/usr/lib/rhs/control-panel/fetchmailconf.xpm
/usr/lib/rhs/control-panel/fetchmailconf.init
%endif

%changelog
* `date '+%a %b %d %Y'` <$email> ${version}
- See the project NEWS file for recent changes.
EOF
