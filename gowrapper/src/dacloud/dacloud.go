// dacloud.go

package dacloud

/*

#include <dacloud.h>
#include <stdlib.h>

typedef void * dago;
struct dago_cloud {
	struct da_cloud_config dcc;
	char cfile[PATH_MAX];
	int r;
};

typedef struct dago_cloud dago_cloud_t;

dago*
dago_cloud_init(char *cfile)
{
	dago_cloud_t *d = malloc(sizeof(*d));
	strncpy(d->cfile, cfile, sizeof(d->cfile) - 1);
	d->cfile[sizeof(d->cfile) - 1] = '\0';
	return ((dago)d);
}

int
dago_cloud_load(dago _d)
{
	if (_d) {
		dago_cloud_t *d = (dago_cloud_t *)_d;
		return (da_cloud_init(&d->dcc, d->cfile));
	}

	return (-1);
}

void
dago_cloud_free(dago _d)
{
	if (_d) {
		dago_cloud_t *d = (dago_cloud_t *)_d;
		da_cloud_fini(&d->dcc);
		free(d);
	}
}

struct dago_detect {
	struct da_cloud_config *dcc;
	struct da_cloud_header_head h;
	struct da_cloud_property_head p;
	struct da_cloud_property *pp;
	int hr;
};

typedef struct dago_detect dago_detect_t;

dago
dago_cloud_header_init(dago _d)
{
	if (_d) {
		dago_cloud_t *d = (dago_cloud_t *)_d;
		dago_detect_t *dd = malloc(sizeof(*dd));
		dd->dcc = &d->dcc;
		dd->hr = da_cloud_header_init(&dd->h);
		return (dd);
	}

	return (NULL);
}

int
dago_cloud_detect(dago _d)
{
	if (_d) {
		dago_detect_t *d = (dago_detect_t *)_d;
		return (da_cloud_detect(d->dcc, &d->h, &d->p));
	}

	return (-1);
}

void
dago_detect_fini(dago _d)
{
	if (_d) {
		dago_detect_t *d = (dago_detect_t *)_d;
		da_cloud_properties_free(&d->p);
		da_cloud_header_free(&d->h);
	}
}

dago
dago_prop_next(dago _d)
{
	struct da_cloud_property *d = (struct da_cloud_property *)_d;
	return (SLIST_NEXT(d, entries));
}

typedef enum da_cloud_property_type da_cloud_property_type;

da_cloud_property_type
dago_prop_type(dago _d)
{
	struct da_cloud_property *d = (struct da_cloud_property *)_d;
	return (d->type);
}

long
dago_prop_getinteger(dago _d)
{
	struct da_cloud_property *d = (struct da_cloud_property *)_d;
	return (d->value.l);
}

char *
dago_prop_getstring(dago _d)
{
	struct da_cloud_property *d = (struct da_cloud_property *)_d;
	return (d->value.s);
}

typedef struct da_cloud_property da_cloud_property_t;
#cgo LDFLAGS: -L. -L/usr/local/lib -ldacloud
#cgo CFLAGS: -I. -I/usr/local/include
*/
import "C"
import "unsafe"

type DaGo struct {
	dc C.dago;
	R int;
}

func Init(cfile string) DaGo {
	var ret DaGo;
	_cfile := C.CString(cfile);
	ret.dc = (C.dago)(C.dago_cloud_init(_cfile));
	ret.R = (int)(C.dago_cloud_load(ret.dc));
	defer C.free((unsafe.Pointer)(_cfile));
	return ret;	
}

func Detect(f DaGo, hdrs map[string]string) map[string]interface{} {
	ret := make(map[string]interface{});
	det := (*C.dago_detect_t)(C.dago_cloud_header_init(f.dc));

	if (det.hr == 0) {
		for k, v := range hdrs {
			key := C.CString(k);
			val := C.CString(v);

			C.da_cloud_header_add(&det.h, key, val);
		}

		if (C.dago_cloud_detect(det) == 0) {
			for det.pp = det.p.list.slh_first;
			    det.pp != nil;
			    det.pp = (*C.da_cloud_property_t)(C.dago_prop_next(det.pp)) {
				    ptype := (C.da_cloud_property_type)(C.dago_prop_type(det.pp));
				    pkey := C.GoString(det.pp.name);

				    switch (ptype) {
				    case C.DA_CLOUD_LONG:
					    value := C.dago_prop_getinteger(det.pp);
					    ret[pkey] = (int)(value);
					    break;
				    case C.DA_CLOUD_BOOL:
					    value := C.dago_prop_getinteger(det.pp);
					    if value == 1 {
						    ret[pkey] = true;
					    } else {
						    ret[pkey] = false;
					    }
					    break;
				    case C.DA_CLOUD_STRING, C.DA_CLOUD_UNKNOWN:
					    value := C.dago_prop_getstring(det.pp);
					    ret[pkey] = C.GoString(value);
					    break;
				}
			}
		}

		C.dago_detect_fini(det);
	}

	return ret;
}

func Finalize(f DaGo) {
	C.dago_cloud_free(f.dc);
}
