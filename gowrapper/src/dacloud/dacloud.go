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
typedef struct da_cloud_property da_cloud_property_t;

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
dago_cloud_detect(dago_detect_t *d)
{
	if (d) {
		return (da_cloud_detect(d->dcc, &d->h, &d->p));
	}

	return (-1);
}

void
dago_detect_fini(dago_detect_t *d)
{
	if (d) {
		da_cloud_properties_free(&d->p);
		da_cloud_header_free(&d->h);
	}
}

dago
dago_prop_next(da_cloud_property_t *d)
{
	return (SLIST_NEXT(d, entries));
}

typedef enum da_cloud_property_type da_cloud_property_type;

da_cloud_property_type
dago_prop_type(da_cloud_property_t *d)
{
	return (d->type);
}

long
dago_prop_getinteger(da_cloud_property_t *d)
{
	return (d->value.l);
}

char *
dago_prop_getstring(da_cloud_property_t *d)
{
	return (d->value.s);
}

#cgo LDFLAGS: -L. -L/usr/local/lib -ldacloud
#cgo CFLAGS: -I. -I/usr/local/include
*/
import "C"
import (
	"math"
	"runtime"
	"unsafe"
)

type DaGo struct {
	dc C.dago
	R  int
}

type DaError struct {
	msg string
}

func newError(msg string) DaError {
	a := DaError{msg: msg}
	return a
}

func (a DaError) Error() string {
	return a.msg
}

// Initialize cloud API data with a config file path
func Init(cfile string) *DaGo {
	ret := &DaGo{}
	runtime.SetFinalizer(ret, Finalize)
	_cfile := C.CString(cfile)
	ret.dc = (C.dago)(C.dago_cloud_init(_cfile))
	ret.R = (int)(C.dago_cloud_load(ret.dc))
	defer C.free((unsafe.Pointer)(_cfile))
	return ret
}

// Proceeds to the cloud service request and
// if succesful, returns the properties set
func Detect(f *DaGo, hdrs map[string]string) (map[string]interface{}, error) {
	ret := make(map[string]interface{})
	if f == nil || (*f).dc == nil {
		return ret, newError("dacloud.DaGo instance needs to be created by Init")
	}

	det := (*C.dago_detect_t)(C.dago_cloud_header_init((*f).dc))

	if det.hr == 0 {
		for k, v := range hdrs {
			var key [64]C.char
			var val [256]C.char
			bkey := []byte(k)
			bval := []byte(v)
			ksz := (C.size_t)(math.Min(float64(len(bkey)), float64(unsafe.Sizeof(key))))
			vsz := (C.size_t)(math.Min(float64(len(bkey)), float64(unsafe.Sizeof(key))))
			C.memcpy(unsafe.Pointer(&key[0]), unsafe.Pointer(&bkey[0]), ksz)
			C.memcpy(unsafe.Pointer(&val[0]), unsafe.Pointer(&bval[0]), vsz)
			key[ksz] = 0
			val[vsz] = 0

			C.da_cloud_header_add(&det.h, &key[0], &val[0])
		}

		if C.dago_cloud_detect(det) == 0 {
			for det.pp = det.p.list.slh_first; det.pp != nil; det.pp = (*C.da_cloud_property_t)(C.dago_prop_next(det.pp)) {
				ptype := (C.da_cloud_property_type)(C.dago_prop_type(det.pp))
				pkey := C.GoString(det.pp.name)

				switch ptype {
				case C.DA_CLOUD_LONG:
					value := C.dago_prop_getinteger(det.pp)
					ret[pkey] = (int)(value)
					break
				case C.DA_CLOUD_BOOL:
					value := C.dago_prop_getinteger(det.pp)
					if value == 1 {
						ret[pkey] = true
					} else {
						ret[pkey] = false
					}
					break
				case C.DA_CLOUD_STRING, C.DA_CLOUD_UNKNOWN:
					value := C.dago_prop_getstring(det.pp)
					ret[pkey] = C.GoString(value)
					break
				}
			}
		} else {
			err := newError("lookup failed")
			return ret, err
		}

		C.dago_detect_fini(det)
	} else {
		err := newError("headers initialization failed")
		return ret, err
	}

	return ret, nil
}

// OO's variant of Detect
func (f *DaGo) Detect(hdrs map[string]string) (map[string]interface{}, error) {
	return Detect(f, hdrs)
}

// Destructor eventually called by the GC
func Finalize(f *DaGo) {
	if f != nil && (*f).dc != nil {
		C.dago_cloud_free((*f).dc)
	}
}

// OO' variant of Finalize
func (f *DaGo) Finalize() {
	Finalize(f)
}
