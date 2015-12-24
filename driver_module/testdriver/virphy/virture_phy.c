#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/phy.h>

#define VIR_PHY_ID 0x1f1f1f1f
static int mdiophy_fixup_soft_reset(struct phy_device *phydev)
{
	printk("zz %s \n", __func__);
	return 0;
}

static struct phy_driver mdiophy_fixup_driver = {
	.phy_id = VIR_PHY_ID,
	.phy_id_mask = 0xfffffff0,
	.name = "mdiophy_fixup",
	.soft_reset	= mdiophy_fixup_soft_reset,
	.config_init	= genphy_config_init,
	.features	= 0,
	.config_aneg	= genphy_config_aneg,
	.aneg_done	= genphy_aneg_done,
	.read_status	= genphy_read_status,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
	.driver		= { .owner = THIS_MODULE, },
};

static struct phy_device *phy;
static int virtual_probe(struct platform_device *pdev)
{
	struct mii_bus *mdio_bus;
	struct device_node *node;
	struct phy_c45_device_ids c45_ids = {0};
	int rc;


	node = of_find_node_by_name(NULL, "mdio");
	if (!node) {
		dev_err(&pdev->dev, "no find mdio node\n");
		return 0;
	}

	mdio_bus = of_mdio_find_bus(node);
	if (!mdio_bus) {
		dev_err(&pdev->dev, "no find mdio bus\n");
		return 0;
	}

	phy = phy_device_create(mdio_bus, 2, VIR_PHY_ID, 0, &c45_ids);
	if (mdio_bus->irq)
		phy->irq = mdio_bus->irq[2];
	phy->dev.of_node = pdev->dev.of_node;

	/* All data is now stored in the phy struct;
	 * register it */
	rc = phy_device_register(phy);
	if (rc) {
		goto out;
	}
	phy->state = PHY_UP;
	phy->speed = SPEED_1000;
	phy->duplex = DUPLEX_FULL;
	phy->autoneg = AUTONEG_DISABLE;
	phy->interface = PHY_INTERFACE_MODE_RGMII;

	rc = phy_driver_register(&mdiophy_fixup_driver);
	if (rc) {
		dev_err(&pdev->dev, "midophy fixup driver register failed\n");
		goto out;
	}

	dev_dbg(&pdev->dev, "midophy fixup driver init\n");
	return 0;
out:
	phy_device_free(phy);
	return 1;

}

static int virtual_remove(struct platform_device *ofdev)
{
	phy_device_free(phy);
	phy_driver_unregister(&mdiophy_fixup_driver);
	return 0;
}

static struct of_device_id phy_match[] =
{
	{.compatible = "iot,l2switch-phy",},
	{},
};
MODULE_DEVICE_TABLE(of, phy_match);

static struct platform_driver vir_phy_driver = {
	.driver = {
		.name = "mdiophy_fixup",
		.owner = THIS_MODULE,
		.of_match_table = phy_match,
	},
	.probe = virtual_probe,
	.remove = virtual_remove,
};

module_platform_driver(vir_phy_driver);

MODULE_DESCRIPTION("Mdio PHY fixup driver");
MODULE_LICENSE("GPL v2");
