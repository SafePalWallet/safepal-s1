#define LOG_TAG "wallet_util_hw"

#include <unistd.h>
#include "common_c.h"
#include "wallet_util.h"
#include "secure_api.h"
#include "sha3.h"
#include "device.h"
#include "rand.h"
#include "win_comm.h"

#define RANDOM_DIGEST_LENGTH 32

int random_buffer(uint8_t *buf, size_t len) {
	FILE *f = fopen("/dev/urandom", "rb");
	if (f == NULL) {
		db_error("open random false");
		return -1;
	}
	if (fread(buf, 1, len, f) != len) {
		fclose(f);
		db_error("read random false");
		return -1;
	}
	fclose(f);
	//db_secure("random buf:%s", debug_ubin_to_hex(buf, len));
	return len;
}

static int _mix_random_digest(uint8_t *randata, size_t len) {
	int ret;
	unsigned char buff[64];
	SHA3_CTX context;
	if (len != RANDOM_DIGEST_LENGTH) {
		db_serr("invalid len:%d", len);
		return -1;
	}
	sha3_256_Init(&context);
	ret = sechip_get_id(buff);
	if (ret > 0) {
		sha3_Update(&context, buff, ret);
	}
	if (random_buffer(buff, 64) != 64) {
		db_serr("gen sys random false");
		return -1;
	}
	sha3_Update(&context, buff, 64);

	ret = device_get_diviceid((char *) buff, sizeof(buff));
	if (ret > 0) {
		sha3_Update(&context, buff, ret); //device id
	}
	sha3_Update(&context, (const unsigned char *) &Global_Key_Random_Source, sizeof(Global_Key_Random_Source));

	if (sapi_get_random(buff, 64) != 64) {
		db_serr("gen sapi random false");
		return -1;
	}
	sha3_Update(&context, buff, 64);
	sha3_Final(&context, randata);
	memset(buff, 0, sizeof(buff));
	return len;
}

int get_mix_random_buffer(uint8_t *buf, size_t len) {
	uint8_t tmpbuf[RANDOM_DIGEST_LENGTH];
	int left = len;
	uint8_t *p = buf;
	while (left > 0) {
		if (_mix_random_digest(tmpbuf, RANDOM_DIGEST_LENGTH) != RANDOM_DIGEST_LENGTH) {
			db_serr("gen random false");
			return -1;
		}
		memcpy(p, tmpbuf, left > RANDOM_DIGEST_LENGTH ? RANDOM_DIGEST_LENGTH : left);
		left -= RANDOM_DIGEST_LENGTH;
		p += RANDOM_DIGEST_LENGTH;
	}
	memset(tmpbuf, 0, sizeof(tmpbuf));
	db_secure("mix random %d : %s", len, debug_ubin_to_hex(buf, len));
	return len;
}

int get_message_process_winid(const ProtoClientMessage *msg) {
	int winid;
	if (msg->type > 0x5 && msg->type < QR_MSG_INIT_BASE) {
		if (msg->type % 2 == 0) {
			return WINDOWID_TX_VERIFY_CODE;
		} else {
			db_error("invalid msg type:%d", msg->type);
			return 0;
		}
	}
	switch (msg->type) {
		case QR_MSG_BIND_ACCOUNT_REQUEST:
		case QR_MSG_GET_PUBKEY_REQUEST:
		case QR_MSG_USER_ACTIVE:
			winid = WINDOWID_QRPROC;
			break;
		default:
			db_error("invalid msg type:%d", msg->type);
			winid = 0;
	}
	return winid;
}

int get_coin_icon_path(int type, const char *uname, char *path, int size) {
	if (!uname || !path || size < 40) {
		if (path) *path = 0;
		return -1;
	}

    if (type < COIN_TYPE_NFT_BASE) {
        if (!IS_VALID_COIN_TYPE(type)) {
            return -2;
        }
    }

	uname = get_coin_main_uname(type, uname);
	const char *res = get_system_res_point();
	int ret;
	if (type == COIN_TYPE_ERC20 && !strcmp(uname, "SFP")) {
		ret = snprintf(path, size, "%s/img/coin/SFP_ERC20.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
    } else if (type == COIN_TYPE_BEP20) {
        if (!strcmp(uname, "SFP")) {
            ret = snprintf(path, size, "%s/img/coin/SFP_BEP20.png", res);
            if (access(path, F_OK) == 0) {
                return ret;
            }
        } else if (!strcmp(uname, "BNB")) {
            ret = snprintf(path, size, "%s/img/coin/%s.png", res, uname);
            if (access(path, F_OK) == 0) {
                return ret;
            }
        } else {
            ret = snprintf(path, size, "%s/img/coin/%s_%d.png", res, uname, type);
            if (access(path, F_OK) == 0) {
                return ret;
            }
        }
	} else if (type == COIN_TYPE_SUI_TEST && !strcmp(uname, "SUI")) {
		ret = snprintf(path, size, "%s/img/coin/SUI_test.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
	} else if (type == COIN_TYPE_PZK && !strcmp(uname, "ETH")) {
		ret = snprintf(path, size, "%s/img/coin/Polygon_zkEVM_eth.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
	} else if (type == COIN_TYPE_ZKSYNC && !strcmp(uname, "ETH")) {
		ret = snprintf(path, size, "%s/img/coin/zkSync_eth.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
	} else if (type == COIN_TYPE_BASE && !strcmp(uname, "ETH")) {
		ret = snprintf(path, size, "%s/img/coin/BASE_eth.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
	} else if (type == COIN_TYPE_LINEA && !strcmp(uname, "ETH")) {
		ret = snprintf(path, size, "%s/img/coin/LINEA_eth.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
	} else if (type == COIN_TYPE_OPBNB && !strcmp(uname, "BNB")) {
		ret = snprintf(path, size, "%s/img/coin/opBNB.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
	} else if (type == COIN_TYPE_BRC20) {
		ret = snprintf(path, size, "%s/img/coin/BTC.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
    } else if ((type == COIN_TYPE_ZKFAIR && !strcmp(uname, "ZKF")) ||
               (type == COIN_TYPE_ZKFAIR && !strcmp(uname, "USDC"))) {
        ret = snprintf(path, size, "%s/img/coin/ZKF.png", res);
        if (access(path, F_OK) == 0) {
            return ret;
        }
    } else if (type == COIN_TYPE_BLAST && !strcmp(uname, "ETH")) {
        ret = snprintf(path, size, "%s/img/coin/BLAST.png", res);
        if (access(path, F_OK) == 0) {
            return ret;
        }
    } else if (type == COIN_TYPE_MANTA && !strcmp(uname, "ETH")) {
        ret = snprintf(path, size, "%s/img/coin/MANTA.png", res);
        if (access(path, F_OK) == 0) {
            return ret;
        }
    } else if (type == COIN_TYPE_MERLIN && !strcmp(uname, "BTC")) {
        ret = snprintf(path, size, "%s/img/coin/MERLIN.png", res);
        if (access(path, F_OK) == 0) {
            return ret;
        }
    } else if (type == COIN_TYPE_TAIKO && !strcmp(uname, "ETH")) {
        ret = snprintf(path, size, "%s/img/coin/TAIKO.png", res);
        if (access(path, F_OK) == 0) {
            return ret;
		}
	} else if (type == COIN_TYPE_SCROLL && !strcmp(uname, "ETH")) {
		ret = snprintf(path, size, "%s/img/coin/SCROLL.png", res);
		if (access(path, F_OK) == 0) {
			return ret;
		}
    } else if (type == COIN_TYPE_BITLAYER && !strcmp(uname, "BTC")) {
        ret = snprintf(path, size, "%s/img/coin/BTC_Bitlayer.png", res);
        if (access(path, F_OK) == 0) {
            return ret;
        }
    }

    if (type >= COIN_TYPE_NFT_BASE && type < COIN_TYPE_UNIVERSAL_EVM_BASE) {
        int original_type = type - COIN_TYPE_NFT_BASE;
        if (original_type == COIN_TYPE_NEAR) {
            ret = snprintf(path, size, "%s/img/coin/NEAR_NFT.png", res);
            if (access(path, F_OK) == 0) {
                return ret;
            }
        } else if (original_type == COIN_TYPE_BRC20) {
            ret = snprintf(path, size, "%s/img/coin/BRC20.png", res);
            if (access(path, F_OK) == 0) {
                return ret;
            }
        } else if (original_type == COIN_TYPE_RUNE_TEST) {
            ret = snprintf(path, size, "%s/img/coin/RUNE_TEST.png", res);
            if (access(path, F_OK) == 0) {
                return ret;
            }
        } else if (original_type == COIN_TYPE_RUNE) {
            ret = snprintf(path, size, "%s/img/coin/RUNE.png", res);
            if (access(path, F_OK) == 0) {
                return ret;
            }
        }
    } else if (type > COIN_TYPE_UNIVERSAL_EVM_BASE) {
        ret = snprintf(path, size, "%s/img/coin/EVM.png", res);
        if (access(path, F_OK) == 0) {
            return ret;
        }
    }

    ret = snprintf(path, size, "%s/img/coin/%s.png", res, uname);
	if (access(path, F_OK) == 0) {
		return ret;
	}
	const char *p = "";
	switch (type) {
		case COIN_TYPE_BITCOIN_TEST:
			p = "tBTC";
			break;
		case COIN_TYPE_RUNE:
			p = "RUNE";
			break;		
		case COIN_TYPE_RUNE_TEST:
			p = "RUNE_TEST";
			break;
		case COIN_TYPE_ERC20:
			p = "ERC20";
			break;
		case COIN_TYPE_ERC721:
		case COIN_TYPE_ERC1155:
			p = "NFT";
			break;
		case COIN_TYPE_BEP20:
			p = "BEP20";
			break;
		case COIN_TYPE_BEP721:
		case COIN_TYPE_BEP1155:
			p = "NFT_BSC";
			break;
		case COIN_TYPE_ETC20:
			p = "ETC20";
			break;
		case COIN_TYPE_BNC:
			p = "BEP2";
			break;
		case COIN_TYPE_TRC10:
			p = "TRC10";
			break;
		case COIN_TYPE_TRC20:
			p = "TRC20";
			break;
		case COIN_TYPE_NEO:
			p = "NEP5";
			break;
		case COIN_TYPE_EOS:
			p = "EOSTOKEN";
			break;
		case COIN_TYPE_XLM:
			p = "STELLAR";
			break;
		case COIN_TYPE_VET:
			p = "VETtoken";
			break;
		case COIN_TYPE_THETA:
			p = "THETA";
			break;
		case COIN_TYPE_POLYGON:
			p = "POLYGON";
			break;
		case COIN_TYPE_POLYGON721:
		case COIN_TYPE_POLYGON1155:
			p = "POLYGON_NFT";
			break;
		case COIN_TYPE_SOLANA:
			p = "SPL";
			break;
		case COIN_TYPE_FANTOM:
			p = "fantom";
			break;
		case COIN_TYPE_HECO:
			p = "HRC_20";
			break;
		case COIN_TYPE_OPTIMISM:
			p = "optimism";
			break;
		case COIN_TYPE_ARBITRUM:
			p = "arbitrum";
			break;
		case COIN_TYPE_ZKSYNC:
			p = "zkSync";
			break;
		case COIN_TYPE_PZK:
			p = "Polygon_zkEVM";
			break;
		case COIN_TYPE_BASE:
			p = "BASE_token";
			break;
		case COIN_TYPE_LINEA:
			p = "LINEA_token";
			break;
		case COIN_TYPE_AVAX:
			p = "AVAX_C-Chain";
			break;
		case COIN_TYPE_CKB:
			break;
		case COIN_TYPE_BOBA:
			p = "ETH_BOBA";
			break;
		case COIN_TYPE_SONGBIRD:
			p = "songbird";
			break;
		case COIN_TYPE_GODWOKEN:
			p = "godwoken";
			break;
		case COIN_TYPE_FANTOM721:
		case COIN_TYPE_HECO721:
		case COIN_TYPE_OPTIMISM721:
		case COIN_TYPE_ARBITRUM721:
		case COIN_TYPE_AVAX721:
        case COIN_TYPE_GODWOKENV1_721:
        case COIN_TYPE_KLAYTN_721:
        case COIN_TYPE_CUSTOM_EVM_721:
        case COIN_TYPE_CORE_721:
        case COIN_TYPE_OPBNB_721:
        case COIN_TYPE_ZKFAIR_721:
        case COIN_TYPE_BLAST_721:
        case COIN_TYPE_MANTA_721:
        case COIN_TYPE_MERLIN_721:
        case COIN_TYPE_BOUNCEBIT_721:
        case COIN_TYPE_AIRDAO_721:
        case COIN_TYPE_GRAVITY_721:
        case COIN_TYPE_TAIKO_721:
        case COIN_TYPE_KAVA_721:
        case COIN_TYPE_SCROLL_721:
        case COIN_TYPE_SEIEVM_721:
        case COIN_TYPE_OASISEVM_721:
        case COIN_TYPE_SONIC_721:
        case COIN_TYPE_LUMIA_721:
        case COIN_TYPE_BITLAYER_721:
        case COIN_TYPE_BERA_721:
        case COIN_TYPE_VICTION_725:
        case COIN_TYPE_VALUECHAIN_721:
		case COIN_TYPE_FANTOM1155:
		case COIN_TYPE_HECO1155:
		case COIN_TYPE_OPTIMISM1155:
		case COIN_TYPE_ARBITRUM1155:
		case COIN_TYPE_AVAX1155:
        case COIN_TYPE_GODWOKENV1_1155:
        case COIN_TYPE_KLAYTN_1155:
        case COIN_TYPE_CUSTOM_EVM_1155:
        case COIN_TYPE_CORE_1155:
		case COIN_TYPE_OPBNB_1155:
        case COIN_TYPE_ZKFAIR_1155:
        case COIN_TYPE_BLAST_1155:
        case COIN_TYPE_MANTA_1155:
        case COIN_TYPE_MERLIN_1155:
        case COIN_TYPE_BOUNCEBIT_1155:
        case COIN_TYPE_AIRDAO_1155:
        case COIN_TYPE_GRAVITY_1155:
        case COIN_TYPE_TAIKO_1155:
        case COIN_TYPE_KAVA_1155:
        case COIN_TYPE_SCROLL_1155:
        case COIN_TYPE_SEIEVM_1155:
        case COIN_TYPE_OASISEVM_1155:
        case COIN_TYPE_SONIC_1155:
        case COIN_TYPE_LUMIA_1155:
        case COIN_TYPE_BITLAYER_1155:
        case COIN_TYPE_BERA_1155:
        case COIN_TYPE_VALUECHAIN_1155:
			p = "NFT_EVM";
			break;
        case COIN_TYPE_TERRA:
            p = "Terra_CW20";
            break;
        case COIN_TYPE_INJ:
            p = "INJ_token";
            break;
		case COIN_TYPE_CHR:
            p = "CHR_token";
            break;
	    case COIN_TYPE_NEAR:
            p = "NEAR_token";
            break;
        case COIN_TYPE_KUCOIN:
            p = "KCS_token";
            break;
        case COIN_TYPE_FUSE:
            p = "FUSE_token";
            break;
        case COIN_TYPE_METIS:
            p = "METIS_token";
            break;
        case COIN_TYPE_AURORA:
            p = "AURORAETH_token";
            break;
        case COIN_TYPE_CELO:
            p = "CELO_token";
            break;
        case COIN_TYPE_MOONBEAM:
            p = "GLMR_token";
            break;
        case COIN_TYPE_CRONOS:
            p = "CRO_token";
            break;
        case COIN_TYPE_GNOSIS:
            p = "xDAI_token";
            break;
        case COIN_TYPE_SYSCOIN:
            p = "SYS_token";
            break;
        case COIN_TYPE_RSK:
            p = "RBTC_token";
            break;
		case COIN_TYPE_GODWOKENV1:
			p = "godwoken_v1";
			break;
		case COIN_TYPE_APTOS:
            p = "Aptos_token";
	        break;
		case COIN_TYPE_APTOS_TEST:
			p = "APT_test_token";
			break;
		case COIN_TYPE_SUI:
			p = "SUI_token";
			break;
		case COIN_TYPE_SUI_TEST:
			p = "SUI_test_token";
			break;
        case COIN_TYPE_ETHW:
            p = "ETHW_token";
            break;
		case COIN_TYPE_FLARE:
			p = "FLARE_token";
			break;
	    case COIN_TYPE_XDC:
            p = "XDC_token";
	        break;
		case COIN_TYPE_TON:
            p = "TON_token";
	        break;
        case COIN_TYPE_KLAYTN:
            p = "KLAY_token";
            break;
		case COIN_TYPE_TLOS:
            p = "TLOS_token";
            break;
		case COIN_TYPE_PLS:
            p = "PLS_token";
            break;
		case COIN_TYPE_CMP:
            p = "CMP_token";
            break;
		case COIN_TYPE_OSMO:
            p = "OSMO_token";
            break;
		case COIN_TYPE_MANTLE:
            p = "MNT_token";
            break;
        case COIN_TYPE_CUSTOM_EVM:
            p = "EVM_token";
            break;
        case COIN_TYPE_CORE:
            p = "CORE_token";
            break;
        case COIN_TYPE_OPBNB:
            p = "opBNB_token";
            break;
        case COIN_TYPE_ZKFAIR:
            p = "ZKF_token";
            break;
        case COIN_TYPE_BLAST:
            p = "BLAST_token";
            break;
        case COIN_TYPE_MANTA:
            p = "MANTA_token";
            break;
        case COIN_TYPE_MERLIN:
            p = "MERLIN_token";
            break;
		case COIN_TYPE_EGLD:
			p = "EGLD_token";
			break;
        case COIN_TYPE_BOUNCEBIT:
            p = "BB_token";
            break;
        case COIN_TYPE_AIRDAO:
            p = "AMB_token";
            break;
        case COIN_TYPE_GRAVITY:
            p = "G_token";
            break;
        case COIN_TYPE_TAIKO:
            p = "TAIKO_token";
            break;
		case COIN_TYPE_VENOM:
			p = "VENOM_token";
			break;
        case COIN_TYPE_SCROLL:
            p = "SCROLL_token";
            break;
        case COIN_TYPE_KASPA:
            p = "KAS_token";
            break;
		case COIN_TYPE_SEIEVM:
            p = "SEI_token";
            break;
		case COIN_TYPE_CARDANO:
			p = "ADA_token";
			break;
		case COIN_TYPE_ALPH:
			p = "ALPH_token";
			break;
		case COIN_TYPE_OASISEVM:
			p = "ROSE_token";
			break;
        case COIN_TYPE_KAVA:
            p = "KAVA_token";
            break;
        case COIN_TYPE_SONIC:
            p = "S_token";
            break;
        case COIN_TYPE_LUMIA:
            p = "LUMIA_token";
            break;
        case COIN_TYPE_BITLAYER:
            p = "BTC_Bitlayer_token";
            break;
        case COIN_TYPE_BERA:
            p = "BERA_token";
            break;
        case COIN_TYPE_VICTION:
        case COIN_TYPE_VICTION_21:
        case COIN_TYPE_VICTION_25:
            p = "VIC_token";
            break;
        case COIN_TYPE_VALUECHAIN:
            p = "SOSO_token";
            break;
        default:
			p = "UNKNOW";
	}
	return snprintf(path, size, "%s/img/coin/%s.png", res, p);
}
