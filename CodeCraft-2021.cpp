#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <limits>
#include <algorithm>

using namespace std;

class Server {
private:
    string serverType; // 服务器型号
    int core;          // CPU核数
    int memory;        // 内存大小
    int price;         // 硬件成本
    int cost;          // 每日能耗成本

    int aAvailableCore;   // A节点可用CPU核数
    int aAvailableMemory; // A节点可用内存
    int bAvailableCore;   // B节点可用CPU核数
    int bAvailableMemory; // B节点可用内存
public:
    Server(const string &serverType, int core, int memory, int price, int cost) {
        try {
            if (core % 2 != 0 || memory % 2 != 0) {
                throw "Server requires even core and memory requirements.";
            }
        }
        catch (const char *msg) {
            cerr << msg << endl;
        }
        this->serverType = serverType;
        this->core = core;
        this->memory = memory;
        this->price = price;
        this->cost = cost;

        aAvailableCore = core / 2;
        aAvailableMemory = memory / 2;
        bAvailableCore = core / 2;
        bAvailableMemory = memory / 2;
    }

    ~Server() {}

    string getServerType() { return serverType; }

    int getCore() const { return core; }

    int getMemory() const { return memory; }

    int getPrice() const { return price; }

    int getCost() const { return cost; }

    int getAAvailableCore() const { return aAvailableCore; }

    int getAAvailableMemory() const { return aAvailableMemory; }

    int getBAvailableCore() const { return bAvailableCore; }

    int getBAvailableMemory() const { return bAvailableMemory; }

    void addAvailableCore(int num, const string &node) {
        try {
            if (node == "A")
                aAvailableCore += num;
            else if (node == "B")
                bAvailableCore += num;
            else if (node == "AB") {
                aAvailableCore += num;
                bAvailableCore += num;
            } else
                throw "Specify the correct node.";
        }
        catch (const char *msg) {
            cerr << msg << endl;
        }
    }

    void addAvailableMemory(int num, const string &node) {
        try {
            if (node == "A")
                aAvailableMemory += num;
            else if (node == "B")
                bAvailableMemory += num;
            else if (node == "AB") {
                aAvailableMemory += num;
                bAvailableMemory += num;
            } else
                throw "Specify the correct node.";
        }
        catch (const char *msg) {
            cerr << msg << endl;
        }
    }

    bool allocate(int core_num, int memory_num, const string &node) {
        if (node == "A") { // 在A节点上分配
            int remainCore = aAvailableCore - core_num;
            int remainMemory = aAvailableMemory - memory_num;
            if (remainCore >= 0 && remainMemory >= 0) {
                aAvailableCore = remainCore;
                aAvailableMemory = remainMemory;
                return true;
            }
        } else if (node == "B") { // 在B节点上分配
            int remainCore = bAvailableCore - core_num;
            int remainMemory = bAvailableMemory - memory_num;
            if (remainCore >= 0 && remainMemory >= 0) {
                bAvailableCore = remainCore;
                bAvailableMemory = remainMemory;
                return true;
            }
        } else if (node == "AB") { // 在AB两个节点上分配
            int coreOnEachNode = core_num / 2;
            int memoryOnEachNode = memory_num / 2;
            int aRemainCore = aAvailableCore - coreOnEachNode;
            int aRemainMemory = aAvailableMemory - memoryOnEachNode;
            int bRemainCore = bAvailableCore - coreOnEachNode;
            int bRemainMemory = bAvailableMemory - memoryOnEachNode;
            if (aRemainCore >= 0 && aRemainMemory >= 0 && bRemainCore >= 0 && bRemainMemory >= 0) {
                aAvailableCore = aRemainCore;
                aAvailableMemory = aRemainMemory;
                bAvailableCore = bRemainCore;
                bAvailableMemory = bRemainMemory;
                return true;
            }
        }
        return false;
    }
}; // end class Server

struct VM {
    /* data */
    string vmType; // 虚拟机类型
    int core;      // CPU核数
    int memory;    // 内存大小
    bool is_dual;  // 是否为双节点部署

    /* init */
    VM() : core(0), memory(0), is_dual(false) {}

    VM(string vt, int c, int m, bool is_d) : vmType(vt), core(c), memory(m), is_dual(is_d) {}
};

struct VMDeployInfo {  // 部署用
    /* data */
    int serverId; // 部署的服务器id
    string node;  // 部署的节点
    VM vm;        // 虚拟机的信息

    /* init */
    VMDeployInfo(int sid, string n, const VM &v) : serverId(sid), node(n), vm(v.vmType, v.core, v.memory, v.is_dual) {}
};

struct VMInfoInServer {  // 迁移用
    /* data */
    int vmId;     // 部署的虚拟机id
    string node;  // 部署的节点
    VM vm;        // 虚拟机的信息

    /* init */
    VMInfoInServer(int vid, string n, const VM &v) : vmId(vid), node(n), vm(v.vmType, v.core, v.memory, v.is_dual) {}
};

struct MigrateInfo {
    /* data */
    int vmId;           // 虚拟机id
    int serverId;       // 目的服务器id
    string targetNode;  // 目的服务器节点

    /* init */
    MigrateInfo(int vid, int sid, string tn) : vmId(vid), serverId(sid), targetNode(tn) {}
};

class Schedule {
private:
    int totalCost = 0;                               // 最后的总成本
    int nextId = 0;                                  // 服务器ID，都为非负数，每次加一
    int nextTempId = -1;                             // 服务器临时ID，都用负数表示，每次减一
    vector<Server> coreLgMemoryServerVec;            // CPU核数大于内存的类型的服务器向量
    vector<Server> memoryLgCoreServerVec;            // 内存大于CPU核数的类型的服务器向量
    vector<VM> vmTypeVec;                            // 所有类型的虚拟机向量
    unordered_map<int, Server> ownedServerMap;       // 拥有的服务器向量  <服务器id，服务器>
    map<string, int> purchaseServerOneDay;           // 一天中购买的服务器 <类型, 数量>
    unordered_map<int, VMDeployInfo> deployedVM;     // 所有的部署的虚拟机 <虚拟机id, 部署信息>
    vector<pair<int, VMDeployInfo>> deployVMOneDay;  // 一天中部署的虚拟机 <虚拟机id, 部署信息>
    vector<MigrateInfo> migrateInfoOneDay;           // 一天中的迁移信息
    unordered_map<int, vector<VMInfoInServer>> serverOwnedVM;// 服务器上部署的虚拟机信息 <服务器id，<虚拟机信息>>
public:
    VM getVMByType(const string &vmType) {  // 根据虚拟机类型查找虚拟机
        for (const auto &it : vmTypeVec) {
            if (it.vmType == vmType)
                return it;
        }
    }

    Server getServerByType(const string &sType, bool coreLgMemory) {  // 在服务器类型向量中根据服务器type查找服务器
        if (coreLgMemory) {
            for (auto &it: coreLgMemoryServerVec) {
                if (it.getServerType() == sType) {
                    return it;
                }
            }
        } else {
            for (auto &it: memoryLgCoreServerVec) {
                if (it.getServerType() == sType) {
                    return it;
                }
            }
        }
    }

    void purchaseServer(const VM &vm, const int days) {
        bool coreLgMemory;
        string purchaseServerType;  // 要购买的服务器类型名称
        double minCost = numeric_limits<double>::max();  // 定义部署的最小支出

        if (vm.core > vm.memory) {
            coreLgMemory = true;
            for (auto &it: coreLgMemoryServerVec) {
                if (!vm.is_dual && it.getCore() / 2 >= vm.core && it.getMemory() / 2 >= vm.memory) {  // 单节点部署的虚拟机
                    // double cost = addVMCost(it, vm, days);
                    double cost = it.getPrice();   // 选择最便宜的服务器
                    if (cost < minCost) {
                        purchaseServerType = it.getServerType();
                        minCost = cost;
                    }
                }
                if (vm.is_dual && it.getCore() >= vm.core && it.getMemory() >= vm.memory) {  // 双节点部署的虚拟机
                    // double cost = addVMCost(it, vm, days);
                    double cost = it.getPrice();   // 选择最便宜的服务器
                    if (cost < minCost) {
                        purchaseServerType = it.getServerType();
                        minCost = cost;
                    }
                }
            }  // end for
        }  // end if(vm.core > vm.memory)

        if (vm.core <= vm.memory) {
            coreLgMemory = false;
            for (auto &it: memoryLgCoreServerVec) {
                if (!vm.is_dual && it.getCore() / 2 >= vm.core && it.getMemory() / 2 >= vm.memory) {  // 单节点部署的虚拟机
                    // double cost = addVMCost(it, vm, days);
                    double cost = it.getPrice();   // 选择最便宜的服务器
                    if (cost < minCost) {
                        purchaseServerType = it.getServerType();
                        minCost = cost;
                    }
                }
                if (vm.is_dual && it.getCore() >= vm.core && it.getMemory() >= vm.memory) {  // 双节点部署的虚拟机
                    // double cost = addVMCost(it, vm, days);
                    double cost = it.getPrice();   // 选择最便宜的服务器
                    if (cost < minCost) {
                        purchaseServerType = it.getServerType();
                        minCost = cost;
                    }
                }
            }  // end for
        }  // end if(vm.core <= vm.memory)

        Server pServer = getServerByType(purchaseServerType, coreLgMemory);
        ownedServerMap.emplace(nextTempId, pServer);
        nextTempId--;  // 每次id减一
        if (purchaseServerOneDay.find(purchaseServerType) != purchaseServerOneDay.end()) {  // 存在该类型的服务器
            purchaseServerOneDay[purchaseServerType] += 1;
        } else {
            purchaseServerOneDay[purchaseServerType] = 1;
        }
    }

    static double addVMCost(const Server &s, const VM &v, const int days) {
        int sc = s.getCore();      // 服务器CPU核数
        int sm = s.getMemory();    // 服务器内存数
        int sp = s.getPrice();     // 服务器价格
        int se = s.getCost();      // 服务器每日能耗
        int vc = v.core;           // 虚拟机CPU核数
        int vm = v.memory;         // 虚拟机内存数

        return double(sp + se * days) * (double(vc) / double(sc) + double(vm) / double(sm));
    }

    static double addVMLossFunction(const int remainSource, const double deployCost) {
        return 1.0 * remainSource + 0.0 * deployCost;
    }

    void addVM(const string &vmType, const int vmId, const int currDay, const int totalDay) {
        int sid;        // 部署在的服务器id
        string node;    // 部署的节点
        VM vm = getVMByType(vmType);
        bool is_dual = vm.is_dual;
        bool isNeedPurchase = true;  // 是否需要新购买服务器
        double minLoss = numeric_limits<double>::max();  // 定义部署的最小代价

        if (!is_dual) {  // 单节点部署
            if (vm.core > vm.memory) {  // 虚拟机CPU核数大于内存数
                for (auto &it: ownedServerMap) {  // 遍历服务器
                    int serverId = it.first;
                    Server server = it.second;
                    // 在CPU核数大于内存数的服务器节点中找
                    if (server.getBAvailableCore() > server.getBAvailableMemory() &&
                        server.getBAvailableCore() >= vm.core && server.getBAvailableMemory() >= vm.memory) { // B节点
                        int bRemainSource =
                                server.getBAvailableCore() - vm.core + server.getBAvailableMemory() - vm.memory;
                        // 所以如果只考虑成本的话，部署在A节点还是B节点无法判断，二者的成本相同
                        double deployCost = addVMCost(server, vm, totalDay - currDay);
                        double loss = addVMLossFunction(bRemainSource, deployCost);
                        if (loss < minLoss) {
                            isNeedPurchase = false;
                            node = "B";
                            sid = serverId;
                            minLoss = loss;
                        }
                    }
                    if (server.getAAvailableCore() > server.getAAvailableMemory() &&
                        server.getAAvailableCore() >= vm.core && server.getAAvailableMemory() >= vm.memory) {  // A节点
                        int aRemainSource =
                                server.getAAvailableCore() - vm.core + server.getAAvailableMemory() - vm.memory;
                        double deployCost = addVMCost(server, vm, totalDay - currDay);
                        double loss = addVMLossFunction(aRemainSource, deployCost);
                        if (loss < minLoss) {  // 目的是寻找最小代价的部署方案
                            isNeedPurchase = false;
                            node = "A";
                            sid = serverId;
                            minLoss = loss;  // 更新最小代价
                        }
                    }
                }
            } else {  // vm.core <= vm.memory  vm.core==vm.memory的情况要不要单独拿出来？
                for (auto &it: ownedServerMap) {  // 遍历服务器
                    int serverId = it.first;
                    Server server = it.second;
                    // 在CPU核数小于等于内存数的服务器节点中找
                    if (server.getBAvailableCore() <= server.getBAvailableMemory() &&
                        server.getBAvailableCore() >= vm.core && server.getBAvailableMemory() >= vm.memory) { // B节点
                        int bRemainSource =
                                server.getBAvailableCore() - vm.core + server.getBAvailableMemory() - vm.memory;
                        // 所以如果只考虑成本的话，部署在A节点还是B节点无法判断，二者的成本相同
                        double deployCost = addVMCost(server, vm, totalDay - currDay);
                        double loss = addVMLossFunction(bRemainSource, deployCost);
                        if (loss < minLoss) {
                            isNeedPurchase = false;
                            node = "B";
                            sid = serverId;
                            minLoss = loss;
                        }
                    }
                    if (server.getAAvailableCore() <= server.getAAvailableMemory() &&
                        server.getAAvailableCore() >= vm.core && server.getAAvailableMemory() >= vm.memory) {  // A节点
                        int aRemainSource =
                                server.getAAvailableCore() - vm.core + server.getAAvailableMemory() - vm.memory;
                        double deployCost = addVMCost(server, vm, totalDay - currDay);
                        double loss = addVMLossFunction(aRemainSource, deployCost);
                        if (loss < minLoss) {  // 目的是寻找最小代价的部署方案
                            isNeedPurchase = false;
                            node = "A";
                            sid = serverId;
                            minLoss = loss;  // 更新最小代价
                        }
                    }
                }
            }
        }  // end if(!is_dual)

        if (is_dual) {  // 双节点部署
            if (vm.core > vm.memory) {
                for (const auto &it: ownedServerMap) {
                    int serverId = it.first;
                    Server server = it.second;
                    int sACore = server.getAAvailableCore();
                    int sBCore = server.getBAvailableCore();
                    int sAMem = server.getAAvailableMemory();
                    int sBMem = server.getBAvailableMemory();
                    int vmHalfCore = vm.core / 2;
                    int vmHalfMem = vm.memory / 2;
                    if ((sACore > sAMem || sBCore > sBMem) && sACore >= vmHalfCore && sAMem >= vmHalfMem &&
                        sBCore >= vmHalfCore && sBMem >= vmHalfMem) {
                        int aRemainSource = sACore - vmHalfCore + sAMem - vmHalfMem;
                        int bRemainSource = sBCore - vmHalfCore + sBMem - vmHalfMem;
                        int remainSource = aRemainSource > bRemainSource ? bRemainSource : aRemainSource;
                        double deployCost = addVMCost(server, vm, totalDay - currDay);
                        double loss = addVMLossFunction(remainSource, deployCost);
                        if (loss < minLoss) {
                            isNeedPurchase = false;
                            node = "AB";
                            sid = serverId;
                            minLoss = loss;
                        }
                    }
                }
            }  // end if(vm.core > vm.memory)

            if (vm.core <= vm.memory) {
                for (const auto &it: ownedServerMap) {
                    int serverId = it.first;
                    Server server = it.second;
                    int sACore = server.getAAvailableCore();
                    int sBCore = server.getBAvailableCore();
                    int sAMem = server.getAAvailableMemory();
                    int sBMem = server.getBAvailableMemory();
                    int vmHalfCore = vm.core / 2;
                    int vmHalfMem = vm.memory / 2;
                    if ((sACore <= sAMem || sBCore <= sBMem) && sACore >= vmHalfCore && sAMem >= vmHalfMem &&
                        sBCore >= vmHalfCore && sBMem >= vmHalfMem) {
                        int aRemainSource = sACore - vmHalfCore + sAMem - vmHalfMem;
                        int bRemainSource = sBCore - vmHalfCore + sBMem - vmHalfMem;
                        int remainSource = aRemainSource > bRemainSource ? bRemainSource : aRemainSource;
                        double deployCost = addVMCost(server, vm, totalDay - currDay);
                        double loss = addVMLossFunction(remainSource, deployCost);
                        if (loss < minLoss) {
                            isNeedPurchase = false;
                            node = "AB";
                            sid = serverId;
                            minLoss = loss;
                        }
                    }
                }
            }  // end if(vm.core <= vm.memory)
        }  // end if(is_dual)

        /* 即使没有以上条件的服务器，是否再遍历一遍，能部署就部署? */
        if (!isNeedPurchase) {  // 不需要购买新的服务器
            auto it = ownedServerMap.find(sid);
            if (it->second.allocate(vm.core, vm.memory, node)) {  // 返回引用问题
                VMDeployInfo info = VMDeployInfo(sid, node, vm);
                pair<int, VMDeployInfo> deployedVMInfo(vmId, info);
                deployVMOneDay.push_back(deployedVMInfo);
            }
        } else {  // 需要购买服务器
            purchaseServer(vm, totalDay - currDay);
            auto it = ownedServerMap.find(nextTempId + 1);  // 购买服务器後，nextTempId減一，所以買的服務器id爲加一後的值
            int pServerId = it->first;
            if (!is_dual) {  // 单节点直接部署到A节点上
                if (it->second.allocate(vm.core, vm.memory, "A")) {
                    VMDeployInfo info = VMDeployInfo(pServerId, "A", vm);
                    pair<int, VMDeployInfo> deployedVMInfo(vmId, info);
                    deployVMOneDay.push_back(deployedVMInfo);
                }
            }
            if (is_dual) {  // 双节点部署
                if (it->second.allocate(vm.core, vm.memory, "AB")) {
                    VMDeployInfo info = VMDeployInfo(pServerId, "AB", vm);
                    pair<int, VMDeployInfo> deployedVMInfo(vmId, info);
                    deployVMOneDay.push_back(deployedVMInfo);
                }
            }
        }  // end if(isNeedPurchase)
    }


    bool deleteVM(const int vmId) {
        // 首先在当天的部署记录中寻找，当天的部署信息在最后一天转换id之后才并入总的部署信息中
        for (auto itOneDay = deployVMOneDay.begin(); itOneDay != deployVMOneDay.end(); itOneDay++) {
            if (itOneDay->first == vmId) {
                VMDeployInfo info = itOneDay->second;
                VM vm = info.vm;
                string node = info.node;
                int serverId = info.serverId;
                // 释放服务器资源
                auto its = ownedServerMap.find(serverId);
                if (vm.is_dual) {
                    its->second.addAvailableCore(vm.core / 2, node);
                    its->second.addAvailableMemory(vm.memory / 2, node);
                } else {
                    its->second.addAvailableCore(vm.core, node);
                    its->second.addAvailableMemory(vm.memory, node);
                }
                // 删除部署信息
                deployVMOneDay.erase(itOneDay);
                return true;
            }
        }

        // 如果没找到，然后在以前的部署记录中寻找
        auto it = deployedVM.find(vmId);
        if (it == deployedVM.end()) {
            return false;
        } else {
            VMDeployInfo info = it->second;
            VM vm = info.vm;
            string node = info.node;
            int serverId = info.serverId;
            // 释放服务器资源
            auto its = ownedServerMap.find(serverId);
            if (vm.is_dual) {  // node == "AB"
                its->second.addAvailableCore(vm.core / 2, node);
                its->second.addAvailableMemory(vm.memory / 2, node);
            } else {
                its->second.addAvailableCore(vm.core, node);
                its->second.addAvailableMemory(vm.memory, node);
            }
            // 删除部署信息
            deployedVM.erase(vmId);
            for (auto vmInfo = serverOwnedVM.find(serverId)->second.begin();
                 vmInfo != serverOwnedVM.find(serverId)->second.end(); vmInfo++) {
                if (vmInfo->vmId == vmId) {
                    serverOwnedVM.find(serverId)->second.erase(vmInfo);
                    return true;
                }
            }
            return false;
        }
    }

    static double
    calcUtilizationRate(Server &server, const string &node, const int allocateCoreNum, const int allocateMemoryNum) {
        // 分配一定资源后的资源利用率。后两个参数为0表示原服务器的利用率。
        if (node == "A") {
            double ACore = double(server.getCore()) / 2;
            double AMemory = double(server.getMemory()) / 2;
            double coreUtilization = (ACore - server.getAAvailableCore() + allocateCoreNum) / ACore;
            double memoryUtilization = (AMemory - server.getAAvailableMemory() + allocateMemoryNum) / AMemory;
            return coreUtilization + memoryUtilization;
        }
        if (node == "B") {
            double BCore = double(server.getCore()) / 2;
            double BMemory = double(server.getMemory()) / 2;
            double coreUtilization = (BCore - server.getBAvailableCore() + allocateCoreNum) / BCore;
            double memoryUtilization = (BMemory - server.getBAvailableMemory() + allocateMemoryNum) / BMemory;
            return coreUtilization + memoryUtilization;
        }
        if (node == "AB") {
            double ACore, AMemory, BCore, BMemory;
            ACore = BCore = double(server.getCore()) / 2;
            AMemory = BMemory = double(server.getMemory()) / 2;
            double coreUtilization = min((ACore - server.getAAvailableCore() + double(allocateCoreNum) / 2) / ACore,
                                         (BCore - server.getBAvailableCore() + double(allocateCoreNum) / 2) / BCore);
            double memoryUtilization = min(
                    (AMemory - server.getAAvailableMemory() + double(allocateMemoryNum) / 2) / AMemory,
                    (BMemory - server.getBAvailableMemory() + double(allocateMemoryNum) / 2) / BMemory);
            return coreUtilization + memoryUtilization;
        }
        return 0;
    }

    void migrateVM() {
        int migrateLimit = int(deployedVM.size() * 0.005) - 1;
        auto vmit = deployedVM.begin();
        int migrateNum = 0;
        for (; vmit != deployedVM.end() && migrateNum < migrateLimit; vmit++) {
            int serverId = vmit->second.serverId;
            auto originalServer = ownedServerMap.find(serverId);
            VM vm = vmit->second.vm;  // 要迁移的虚拟机
            int vmId = vmit->first;  // 要迁移的虚拟机id
            string node = vmit->second.node;  // 要迁移的虚拟机原来所在的节点
            bool canMigrate = false;  // 是否可以进行迁移
            int targetServerId;  // 目标服务器id
            string targetNode;  // 目标服务器节点
            for (auto &ito: ownedServerMap) {  // 遍历查找可以放置的服务器
                if (ito.first != serverId) {  // 目的服务器与原服务器不同
                    if (vm.is_dual) {  // 双节点部署
                        double currUtilizationRate = calcUtilizationRate(originalServer->second, "AB", 0, 0);
                        if (ito.second.getAAvailableCore() >= vm.core / 2 &&
                            ito.second.getAAvailableMemory() >= vm.memory / 2 &&
                            ito.second.getBAvailableCore() >= vm.core / 2 &&
                            ito.second.getBAvailableMemory() >= vm.memory / 2) {
                            double utilizationRate = calcUtilizationRate(ito.second, "AB", vm.core, vm.memory);
                            if (utilizationRate > currUtilizationRate) {
                                currUtilizationRate = utilizationRate;
                                targetServerId = ito.first;
                                targetNode = "AB";
                                canMigrate = true;
                            }
                        }
                    } else {  // if(!is_dual)
                        double currUtilizationRate = calcUtilizationRate(originalServer->second, node, 0, 0);
                        if (ito.second.getAAvailableCore() >= vm.core &&
                            ito.second.getAAvailableMemory() >= vm.memory) {
                            double utilizationRate = calcUtilizationRate(ito.second, "A", vm.core, vm.memory);
                            if (utilizationRate > currUtilizationRate) {
                                currUtilizationRate = utilizationRate;
                                targetServerId = ito.first;
                                targetNode = "A";
                                canMigrate = true;
                            }
                        }
                        if (ito.second.getBAvailableCore() >= vm.core &&
                            ito.second.getBAvailableMemory() >= vm.memory) {
                            double utilizationRate = calcUtilizationRate(ito.second, "B", vm.core, vm.memory);
                            if (utilizationRate > currUtilizationRate) {
                                currUtilizationRate = utilizationRate;
                                targetServerId = ito.first;
                                targetNode = "B";
                                canMigrate = true;
                            }
                        }
                    }
                }
            }

            if (canMigrate) {
                // 在目标服务器上分配资源
                auto targetServer = ownedServerMap.find(targetServerId);
                targetServer->second.allocate(vm.core, vm.memory, targetNode);

                // 原服務器加上釋放的資源
                if (vm.is_dual) {
                    originalServer->second.addAvailableCore(vm.core / 2, node);
                    originalServer->second.addAvailableMemory(vm.memory / 2, node);
                } else {  // !vm.is_dual
                    originalServer->second.addAvailableCore(vm.core, node);
                    originalServer->second.addAvailableMemory(vm.memory, node);
                }

                // 在serverOwnedVM中将该id服务器上的虚拟机信息清空
                auto vmInfo = serverOwnedVM.find(serverId)->second;
                auto vmi = vmInfo.begin();
                for (;vmi!=vmInfo.end();vmi++){
                    if (vmi->vmId == vmId)
                        break;
                }
                vmInfo.erase(vmi);

                // serverOwnedVM中在目标服务器上添加该虚拟机的信息
                serverOwnedVM.find(targetServerId)->second.emplace_back(vmId, targetNode, vm);

                // 修改部署信息
                auto deployInfo = deployedVM.find(vmId);
                deployInfo->second.serverId = targetServerId;
                deployInfo->second.node = targetNode;

                // 记录迁移信息
                migrateInfoOneDay.emplace_back(vmId, targetServerId, targetNode);

                migrateNum++;
            }
        }
    }


    void convertID() {
        int purchaseNumberOneDay = 0;  // 一天中購買的服務器臺數
        for (auto &ps: purchaseServerOneDay)
            purchaseNumberOneDay += ps.second;

        map<int, int> convertMap;
        for (auto &pServerOneDay: purchaseServerOneDay) {
            string sType = pServerOneDay.first;
            int number = pServerOneDay.second;
            int counter = 0;  // 已经修改counter个sType的服务器id，sType的服务器在一天中有number台
            for (int i = 1; i <= purchaseNumberOneDay && counter < number; i++) {
                auto serverInfo = ownedServerMap.find(nextTempId + i);
                if (serverInfo != ownedServerMap.end() && serverInfo->second.getServerType() == sType) {
                    int newID = nextId;
                    nextId++;
                    int oldID = serverInfo->first;
                    Server s = serverInfo->second;
                    ownedServerMap.emplace(newID, s);
                    ownedServerMap.erase(oldID);
                    convertMap.emplace(oldID, newID);
                    counter++;
                }
            }
        }
        // 将获得的转换字典用于修改当天的部署信息
        auto it = deployVMOneDay.begin();
        for (; it != deployVMOneDay.end(); it++) {
            int oldID = it->second.serverId;
            if (oldID > -1)  // 部署在以前的服务器上不用更改id
                continue;
            else
                it->second.serverId = convertMap[oldID];
        }
    }

    void main() {

        int serverNumber;
        cin >> serverNumber;
        for (int i = 0; i < serverNumber; i++) {
            string delimiter;  // 把int后的逗号或括号读出来

            string serverType;
            cin >> serverType;
            serverType.erase(0, 1); // 去除左括号
            serverType.pop_back();  // 去除逗号

            int core;
            cin >> core;
            cin >> delimiter;

            int memory;
            cin >> memory;
            cin >> delimiter;

            int price;
            cin >> price;
            cin >> delimiter;

            int cost;
            cin >> cost;
            cin >> delimiter;

            Server server = Server(serverType, core, memory, price, cost);

            if (core > memory && core - memory >= 60 && core - memory <= 120) { // CPU核数大于内存数的服务器类型
                coreLgMemoryServerVec.push_back(server);
            }
            if (memory >= core && memory - core >= 120 && memory - core <= 180) { // 内存数大于等于CPU核数的服务器类型
                memoryLgCoreServerVec.push_back(server);
            }
        }

        int vmNumber;
        cin >> vmNumber;
        for (int i = 0; i < vmNumber; i++) {
            string delimiter;  // 把int后的逗号或括号读出来

            string vmType;
            cin >> vmType;
            vmType.erase(0, 1);  // 去除左括号
            vmType.pop_back();   // 去除逗号

            int core;
            cin >> core;
            cin >> delimiter;

            int memory;
            cin >> memory;
            cin >> delimiter;

            int is_dual;
            cin >> is_dual;
            cin >> delimiter;

            VM vm = VM(vmType, core, memory, is_dual);

            vmTypeVec.push_back(vm);
        }

        int days;
        cin >> days;
        for (int day = 0; day < days; day++) {
            // 在一天的请求开始前进行迁移

            migrateVM();


            int questNumber;
            cin >> questNumber;
            for (int j = 0; j < questNumber; j++) {
                string delimiter;  // 把int后的逗号或括号读出来

                string questType;
                cin >> questType;
                questType.erase(0, 1);  // 去除左括号
                questType.pop_back();           // 去除逗号

                if (questType == "add") {
                    string vmType;
                    cin >> vmType;
                    vmType.pop_back();         // 去除逗号

                    int vmId;
                    cin >> vmId;
                    cin >> delimiter;

                    addVM(vmType, vmId, day, days);
                }  // end add quest

                if (questType == "del") {
                    int vmId;
                    cin >> vmId;
                    cin >> delimiter;

                    deleteVM(vmId);
                }  // end del quest
            }  // end for (int j = 0; j < questNumber; j++)

            // 为当天的服务器转换id
            convertID();

            // 将当天的部署信息合并到总的部署信息中
            for (auto &vmOneDay: deployVMOneDay) {
                int vmId = vmOneDay.first;
                VMDeployInfo info = vmOneDay.second;
                deployedVM.emplace(vmId, info);
            }

            // 将该天的部署信息加入到服务器拥有的虚拟机信息
            for (auto &it: deployVMOneDay) {
                int vmId = it.first;
                int serverId = it.second.serverId;
                VMInfoInServer vmInfoInServer = VMInfoInServer(vmId, it.second.node, it.second.vm);
                auto server = serverOwnedVM.find(serverId);
                if (server == serverOwnedVM.end()) {  // 如果没有该服务器
                    vector<VMInfoInServer> vmInfoInServerVec;
                    vmInfoInServerVec.push_back(vmInfoInServer);
                    serverOwnedVM.emplace(serverId, vmInfoInServerVec);
                } else {  // 存在该服务器
                    vector<VMInfoInServer> vmInfoInServerVec = server->second;
                    vmInfoInServerVec.push_back(vmInfoInServer);
                    serverOwnedVM[serverId] = vmInfoInServerVec;
                }
            }

            // 输出信息
            cout << "(purchase, " << purchaseServerOneDay.size() << ")\n";
            for (const auto &it : purchaseServerOneDay) {
                cout << "(" << it.first << ", " << it.second << ")\n";
            }
            cout << "(migration, " << migrateInfoOneDay.size() << ")\n";
            for (const auto &it : migrateInfoOneDay) {
                string node = it.targetNode;
                if (node == "AB")
                    cout << "(" << it.vmId << ", " << it.serverId << ")\n";
                else
                    cout << "(" << it.vmId << ", " << it.serverId << ", " << node << ")\n";
            }
            for (const auto &it : deployVMOneDay) {
                VMDeployInfo info = it.second;
                if (info.node == "AB") {
                    cout << "(" << info.serverId << ")\n";
                } else {
                    cout << "(" << info.serverId << ", " << info.node << ")\n";
                }
            }

            // 清空当天变量
            purchaseServerOneDay.clear();
            deployVMOneDay.clear();
            migrateInfoOneDay.clear();
        }
    }

}; //end class Schedule

int main() {
    Schedule schedule = Schedule();
    schedule.main();
    return 0;
}