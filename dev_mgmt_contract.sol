pragma solidity ^0.5.12;

//TODO: Can all functions be non-payable? How to handle ether? SOLUTION: On miners: "geth ... --gasprice 0 ..." https://stackoverflow.com/questions/49318479/how-to-make-sure-transactions-take-0-fee-in-a-private-ethereum-blockchain
//TODO?: Implement log of events?
//TODO?: May not be needed: 0 Cannot be a region ID -- If the region is not active, then just ignore this value, for if it is active it will surely be in activeMangementRegions
//TODO: Link redgion indexes in devices to mgmt Regions
//TODO: Link activeManagementRegionsIndex to activeManagementRegions
//TODO: Link authorizedAdminUsersIndex to activeAdminUsers
//TODO: Deactivate region/reactivate region & delete region (Consider implications of Device.mangementRegionIndexes)
//TODO: Initial admin user
//TODO: Function to change region of device
//TODO: getters for Regions
//TODO: getters for all structs


/**
 *
 */
contract DeviceMgmt {

    enum AddrType {NULL, IP, LORA, ZIGBEE, BLUETOOTH, OTHER} //TODO: Add other common protocols and custom values


    /**
     *
     */
    struct Device {
        bytes16 name;                           // Arbitrary, admin assigned, human readable name
        uint creationTimestamp;                 // Time this device was first created
        bool isAuthd;                           // Is this a valid & authorized device mapping
        string data;                            // Data can point to swarm address or just contain raw IoT device data such as sensor information (encrypted)

        AddrType addrType;                      // How the device connects to the network (IP (ethernet, WiFi, GSM/LTE), LoRa, etc.)
        string addr;                            // Device is to maintain its current IP (v4/v6) address
        string mac;                             // MAC address for further identification

        string publicKey;                       // Devices public encryption key
        uint32 managementRegion;                //
        uint32 managementRegionIndex;           //
        uint32 authorizedDevicesIndex;          //
        //mapping (uint32 => uint32) managementRegionIndexes;    // Mapping of ManagementRegion ID to index into device array within that ManagementRegion of this device
    }


    /**
     *
     */
    struct AdminUser {
        bool isAdmin;                           //
        uint32 authorizedAdminUsersIndex;           // The index within activeAdminUsers which points to this AdminUser
    }


    /*
     * Management Regions control which public key IoT nodes encrypt data with after
     * encrypting with their own private key. Management Regions allow for different
     * public keys to be used to ensure a compromised key does not compromise all data.
     */
    struct ManagementRegion {
        byte16 name;                            //
        string publicKey;                       // The public encryption key used for this management region
        address[] devices;                      // Dynamic array of all devices in this management region
        uint32 activeManagementRegionsIndex;    // The index within activeManagementRegions which points to this ManagementRegion
        bool isActive;                          // Boolean flag to deactivate this region. Devices in a deactivated region will default to index 0
    }

    mapping (address => Device) clientMapping;

    mapping (address => AdminUser) adminMapping;
    address[] activeAdminUsers;

    mapping (uint32 => ManagementRegion) managementRegions;
    uint32 newManagementRegionIndex;
    uint32[] activeManagementRegions;

    address[] authorizedDevices;


    /**
     * @dev
     */
    modifier _authorized {
        require(clientMapping[msg.sender].isAuthd || adminMapping[msg.sender].isAdmin);
        _;
    }


    /**
     * @dev
     */
    modifier _authorizedDeviceOnly {
        require(clientMapping[msg.sender].isAuthd);
        _;
    }


    /**
     * @dev
     */
    modifier _admin {
        require(adminMapping[msg.sender].isAdmin);
        _;
    }


    /**
     * @dev
     */
    function DeviceMgmt() public {

    }


    // Fallback function -- Allow this contract to accept ether
    function() external payable {}


     /**
      * @dev Is the given address an admin? Only for external calls. For internal use, test: adminMapping[clientAddr].isAdmin
      * @param clientAddr
      * @return
      */
    function is_admin(address clientAddr) external view _authorized returns(bool) {
        return adminMapping[clientAddr].isAdmin;
    }


    /**
     * @dev Is the given address an authorized device? Only for external calls. For internal use, test: clientMapping[clientAddr].isAuthd
     * @param clientAddr
     * @return
     */
    function is_authd(address clientAddr) external view _authorized returns(bool) {
        return clientMapping[clientAddr].isAuthd;
    }


    /**
     * @dev
     * @param clientAddr
     * @return
     */
    function get_key(address clientAddr) external view _authorized returns(string) {
        require(clientMapping[clientAddr].isAuthd);
        return clientMapping[clientAddr].publicKey;
    }


    /**
     * @dev
     * @param clientAddr
     * @return
     */
    function get_addrtype(address clientAddr) external view _authorized returns(uint) {
        require(clientMapping[clientAddr].isAuthd);
        return uint(clientMapping[clientAddr].addrType);
    }


    /**
     * @dev
     * @param clientAddr
     * @return
     */
    function get_addr(address clientAddr) external view _authorized returns(string) {
        require(clientMapping[clientAddr].isAuthd);
        return clientMapping[clientAddr].addr;
    }


    /**
     * @dev
     * @param clientAddr
     * @return
     */
    function get_mac(address clientAddr) external view _authorized returns(string) {
        require(clientMapping[clientAddr].isAuthd);
        return clientMapping[clientAddr].mac;
    }


    /**
     * @dev
     * @param clientAddr
     * @return
     */
    function get_data(address clientAddr) external view _authorized returns(string) {
        require(clientMapping[clientAddr].isAuthd);
        return clientMapping[clientAddr].data;
    }


    /**
     * @dev
     * @param clientAddr
     * @param name
     * @param mac
     * @param publicKey
     * @return
     */
    function add_device(address clientAddr, bytes16 name, string mac, string publicKey, uint32 managementRegion) external _admin returns(bool) {
        require(managementRegions[managementRegions].isActive);
        var device = clientMapping[clientAddr];
        require(device.publicKey == ""); //TODO: Find better way to do this

        device.name = name;
        device.addrType = UNSET;
        device.addr = "";
        device.mac = mac;
        device.publicKey = publicKey;
        device.creationTimestamp = now;
        device.managementRegion = managementRegion;
        //TODO: Add device to management region device list and assign index
        //device.managementRegionIndex =
        device.isAuthd = true;
        return true;
    }


    /**
     * @dev
     * @param clientAddr
     * @return
     */
    function deauth_device(address clientAddr) external _admin returns(bool) {
        require(clientMapping[clientAddr].isAuthd);

        var device = clientMapping[clientAddr];
        //TODO: Remove device from its regions indexes
        //TODO: Remove device from active device index
        device.isAuthd = false;
        return true;
    }


    /**
     * @dev
     * @param newPublicKey
     * @return
     */
    function update_publickey(string newPublicKey) external _authorizedDeviceOnly returns(bool) {
        clientMapping[msg.sender].publicKey = newPublicKey;
        return true;
    }


    /**
     * @dev
     * @param addrType
     * @param addr
     * @return
     */
    function update_addr(uint addrType, string addr) external _authorizedDeviceOnly returns(bool) {
        var sender = clientMapping[clientAddr];
        sender.addrType = AddrType(addrType);
        sender.addr = addr;
        return true;
    }


    /**
     * @dev
     * @param clientAddr
     * @param newPublicKey
     * @return
     */
    function admin_update_publickey(address clientAddr, string newPublicKey) external _admin returns(bool) {
        require(clientMapping[clientAddr].isAuthd);

        var device = clientMapping[clientAddr];
        device.publicKey = newPublicKey;
        return true;
    }


    /**
     * @dev
     * @param newAdminAddr
     * @return
     */
    function authorize_admin(address newAdminAddr) external _admin returns(bool) {
        adminMapping[newAdminAddr].isAdmin = true;
        adminMapping[newAdminAddr].authorizedAdminUsersIndex = authorizedAdminUsers.push(newAdminAddr) - 1;
        return true;
    }


    /**
     * @dev
     * @param adminAddr
     * @return
     */
    function deauthorize_admin(address adminAddr) external _admin returns(bool) {
        require(numAdmin > 1);

        if (authorizedAdminUsers.length - 1 == adminMapping[adminAddr].authorizedAdminUsersIndex) {
            // If the index of the admin user is the last item in authorizedAdminUsers, just delete it
            delete authorizedAdminUsers[adminMapping[adminAddr].authorizedAdminUsersIndex];
        } else {
            // If it is not the last item, swap the last item to this index and delete the last item (keep array from fragmenting)
            //TODO: REVIEW THIS LOGIC
            authorizedAdminUsers[adminMapping[adminUser].authorizedAdminUsersIndex] = authorizedAdminUsers[authorizedAdminUsers.length - 1];
            adminMapping[authorizedAdminUsers[authorizedAdminUsers.length - 1]].authorizedAdminUsersIndex = adminMapping[adminAddr].authorizedAdminUsersIndex;
            delete authorizedAdminUsers[authorizedAdminUsers.length - 1];
        }

        adminMapping[adminAddr].authorizedAdminUsersIndex = 0;
        adminMapping[newAdminAddr].isAdmin = false;
        return true;
    }


    /**
     * @dev
     * @return
     */
    function get_num_admin() external view _admin returns(uint16) {
        return activeAdminUsers.length;
    }


    /**
     * @dev Creates a new management region with given name and public key. Sender must be an admin.
     * @param name Name of new management region. No checks for uniqueness are made. Name is only external identification.
     * @param publicKey The public encryption key that IoT devices are to encrypt data with after encrypting with their own private key.
     * @return The assigned index identifier of this manament region. This value is immutable and will not be recycled if the management region is deactivated.
     */
    function create_management_region(byte16 name, string publicKey) external _admin returns(uint32) {
        unit32 memory regionIndex = newManagementRegionIndex++;

        managementRegions[regionIndex].name = name;
        managementRegions[regionIndex].publicKey = publicKey;
        managementRegions[regionIndex].activeManagementRegionsIndex = activeManagementRegions.push(regionIndex) - 1;
        managementRegions[regionIndex].isActive = true;
        return regionIndex;
    }


    /**
     * @dev
     * @param region
     * @return
     */
    function deactivate_management_region(uint32 region) external _admin return(bool) {
        require(managementRegions[region].isActive);
        require(managementRegions[region].devices.length == 0);

        if (activeManagementRegions.length - 1 == managementRegions[region].activeManagementRegionsIndex) {
            // If the index of the management region is the last item in activeManagementRegions, just delete it
            delete activeManagementRegions[managementRegions[region].activeManagementRegionsIndex];
        } else {
            // If it is not the last item, swap the last item to this index and delete the last item (keep array from fragmenting)
            activeManagementRegions[managementRegions[region].activeManagementRegionsIndex] = activeManagementRegions[activeMangementRegions.length - 1];
            managementRegions[activeManagementRegions[activeMangementRegions.length - 1]].activeManagementRegionsIndex = managementRegions[region].activeManagementRegionsIndex;
            delete activeManagementRegions[activeMangementRegions.length - 1];
        }

        managementRegions[region].activeManagementRegionsIndex = 0;
        managementRegions[region].isActive = false;
        return true;
    }


    /**
     * @dev
     * @param region
     * @return
     */
     function get_management_region_devices(uint32 region) view _admin returns(address[]) {
         require(managementRegions[region].isActive);
         return managementRegions[region].devices;
     }


     /**
      * @dev
      * @return
      */
      function get_active_management_regions() view _admin returns(uint32[]) {
          return activeManagementRegions;
      }


      /**
       * @dev
       * @return
       */
       function get_active_admins() view _admin returns(address[]) {
           return activeAdminUsers;
       }


       /**
        * @dev
        * @return
        */
        function get_authorized_devices() view _admin returns(address[]) {
            return authorizedDevices;
        }


        /**
         * @dev
         * @return
         */
         function get_num_active_admins() view _admin returns(uint32) {
             return activeAdminUsers.length;
         }

}
