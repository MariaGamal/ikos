#!/usr/bin/env python

import argparse


def main():
    parser = argparse.ArgumentParser(
        description="Show Your WE Balance"
    )

    parser.add_argument(
        "-n",
        help="Service Number",
        metavar="<SERVICE_NUMBER>",
        required=True
    )
    parser.add_argument(
        "-p",
        help="Password",
        metavar="<PASSWORD>",
        required=True,
    )

    args = parser.parse_args()
    we = MyWe(args.n, args.p)

    print(we.balance)


if __name__ == "__main__":
    main()
