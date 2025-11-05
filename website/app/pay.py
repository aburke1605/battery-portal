# server.py
from flask import Flask, Blueprint, jsonify
import stripe
import os

pay = Blueprint("pay", __name__, url_prefix="/pay")

stripe.api_key = os.getenv("STRIPE_SECRET_KEY")

@pay.route("/create-intent", methods=["POST"])
def create_intent():
    intent = stripe.PaymentIntent.create(
        amount=1099,  # $10.99
        currency="usd",
        automatic_payment_methods={"enabled": True},
    )
    return jsonify({"client_secret": intent.client_secret})
