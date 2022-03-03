package submod

import (
	"fmt"

	"github.com/urfave/cli"
)

var pageCommand = cli.Command{
	Name:        "page",
	Aliases:     []string{"rs"},
	Usage:       "show the config meta date of containerd repo",
	ArgsUsage:   "<src-images>]",
	Description: "show respect of containerd repo",
	Flags: []cli.Flag{
		cli.StringFlag{
			Name:  "cachefile",
			Usage: "add cachefile",
		},
	},
	Action: func(context *cli.Context) error {
		fmt.Println("base:", context.String("basepath"))

		return nil
	},
}
