module github.com/tron/cida

go 1.15

require (
	github.com/containerd/console v1.0.1
	github.com/containerd/containerd v1.5.0-beta.3
	github.com/containerd/continuity v0.0.0-20210208174643-50096c924a4e
	github.com/containerd/go-cni v1.0.2
	github.com/containerd/go-runc v0.0.0-20201020171139-16b287bc67d0 // indirect
	github.com/dgraph-io/ristretto v0.1.0
	github.com/elazarl/goproxy v0.0.0-20210801061803-8e322dfb79c4
	github.com/jstemmer/gotags v1.4.1 // indirect
	github.com/kr/text v0.2.0 // indirect
	github.com/moby/sys/mountinfo v0.4.0
	github.com/niemeyer/pretty v0.0.0-20200227124842-a10e7caefd8e // indirect
	github.com/opencontainers/go-digest v1.0.0
	github.com/opencontainers/image-spec v1.0.1
	github.com/opencontainers/runtime-spec v1.0.3-0.20200929063507-e6143ca7d51d
	github.com/pkg/errors v0.9.1
	github.com/sirupsen/logrus v1.7.0
	github.com/spf13/cobra v1.2.1
	github.com/spf13/viper v1.8.1
	github.com/stretchr/testify v1.7.0
	github.com/urfave/cli v1.22.2
	golang.org/x/sync v0.0.0-20210220032951-036812b2e83c
	golang.org/x/sys v0.0.0-20210510120138-977fb7262007
	google.golang.org/grpc v1.38.0
	gopkg.in/check.v1 v1.0.0-20200227125254-8fa46927fb4f // indirect
)

replace github.com/elazarl/goproxy => github.com/taoting1234/goproxy v0.0.0-20210901033843-ebf581737889
