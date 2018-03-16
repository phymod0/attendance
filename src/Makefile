
SUBDIRS = \
	attendance_server \
	database_server \
	attendance_timer \
	web_app/cgi/index \
	web_app/cgi/mark_attendance \
	web_app/cgi/register_new

all:
	for d in $(SUBDIRS); do $(MAKE) -C $$d; done
clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

